package org.msgpack.jruby;

import java.util.Arrays;

import org.jruby.Ruby;
import org.jruby.RubyClass;
import org.jruby.RubyString;
import org.jruby.RubyObject;
import org.jruby.RubyArray;
import org.jruby.RubyHash;
import org.jruby.RubyNumeric;
import org.jruby.RubyFixnum;
import org.jruby.RubyProc;
import org.jruby.RubyIO;
import org.jruby.exceptions.RaiseException;
import org.jruby.runtime.builtin.IRubyObject;
import org.jruby.runtime.Block;
import org.jruby.runtime.ObjectAllocator;
import org.jruby.runtime.ThreadContext;
import org.jruby.anno.JRubyClass;
import org.jruby.anno.JRubyMethod;
import org.jruby.util.ByteList;
import org.jruby.ext.stringio.StringIO;

import static org.jruby.runtime.Visibility.PRIVATE;

@JRubyClass(name="MessagePack::Unpacker")
public class Unpacker extends RubyObject {
  public ExtRegistry registry;
  private IRubyObject stream;
  private IRubyObject data;
  private Decoder decoder;
  private final RubyClass underflowErrorClass;
  private boolean symbolizeKeys;
  private boolean allowUnknownExt;

  public Unpacker(Ruby runtime, RubyClass type) {
    super(runtime, type);
    this.underflowErrorClass = runtime.getModule("MessagePack").getClass("UnderflowError");
  }

  static class UnpackerAllocator implements ObjectAllocator {
    public IRubyObject allocate(Ruby runtime, RubyClass klass) {
      return new Unpacker(runtime, klass);
    }
  }

  static class ExtRegistry {
    private Ruby runtime;
    public RubyArray[] array;

    public ExtRegistry(Ruby runtime) {
      this.runtime = runtime;
      this.array = new RubyArray[256];
    }

    public ExtRegistry dup() {
      ExtRegistry copy = new ExtRegistry(this.runtime);
      copy.array = Arrays.copyOf(this.array, 256);
      return copy;
    }

    public void put(RubyClass klass, int typeId, IRubyObject proc, IRubyObject arg) {
      IRubyObject k = klass;
      if (k == null) {
        k = runtime.getNil();
      }
      RubyArray e = RubyArray.newArray(runtime, new IRubyObject[] { k, proc, arg });
      array[typeId + 128] = e;
    }

    // proc, typeId(Fixnum)
    public IRubyObject lookup(int type) {
      RubyArray e = array[type + 128];
      if (e == null) {
        return null;
      }
      return e.entry(1);
    }
  }

  @JRubyMethod(name = "initialize", optional = 1, visibility = PRIVATE)
  public IRubyObject initialize(ThreadContext ctx, IRubyObject[] args) {
    symbolizeKeys = false;
    allowUnknownExt = false;
    if (args.length > 0) {
      if (args[args.length - 1] instanceof RubyHash) {
        RubyHash options = (RubyHash) args[args.length - 1];
        IRubyObject sk = options.fastARef(ctx.getRuntime().newSymbol("symbolize_keys"));
        if (sk != null) {
          symbolizeKeys = sk.isTrue();
        }
        IRubyObject au = options.fastARef(ctx.getRuntime().newSymbol("allow_unknown_ext"));
        if (au != null) {
          allowUnknownExt = au.isTrue();
        }
      } else if (!(args[0] instanceof RubyHash)) {
        setStream(ctx, args[0]);
      }
    }
    this.registry = new ExtRegistry(ctx.getRuntime());
    return this;
  }

  public void setExtRegistry(ExtRegistry registry) {
    this.registry = registry;
  }

  public static Unpacker newUnpacker(ThreadContext ctx, ExtRegistry extRegistry, IRubyObject[] args) {
    Unpacker unpacker = new Unpacker(ctx.getRuntime(), ctx.getRuntime().getModule("MessagePack").getClass("Unpacker"));
    unpacker.initialize(ctx, args);
    unpacker.setExtRegistry(extRegistry);
    return unpacker;
  }

  @JRubyMethod(name = "registered_types_internal", visibility = PRIVATE)
  public IRubyObject registeredTypesInternal(ThreadContext ctx) {
    RubyHash mapping = RubyHash.newHash(ctx.getRuntime());
    for (int i = 0; i < 256; i++) {
      if (registry.array[i] != null) {
        mapping.fastASet(RubyFixnum.newFixnum(ctx.getRuntime(), i - 128), registry.array[i].dup());
      }
    }
    return mapping;
  }

  @JRubyMethod(name = "register_type", required = 1, optional = 2)
  public IRubyObject registerType(ThreadContext ctx, IRubyObject[] args, final Block block) {
    // register_type(type){|data| ExtClass.deserialize(...) }
    // register_type(type, Class, :from_msgpack_ext)
    Ruby runtime = ctx.getRuntime();
    IRubyObject type = args[0];

    RubyClass extClass;
    IRubyObject arg;
    IRubyObject proc;
    if (args.length == 1) {
      if (! block.isGiven()) {
        throw runtime.newLocalJumpErrorNoBlock();
      }
      proc = RubyProc.newProc(runtime, block, block.type);
      if (proc == null)
        System.err.println("proc from Block is null");
      arg = proc;
      extClass = null;
    } else if (args.length == 3) {
      extClass = (RubyClass) args[1];
      arg = args[2];
      proc = extClass.method(arg);
    } else {
      throw runtime.newArgumentError(String.format("wrong number of arguments (%d for 1 or 3)", 2 + args.length));
    }

    long typeId = ((RubyFixnum) type).getLongValue();
    if (typeId < -128 || typeId > 127) {
      throw runtime.newRangeError(String.format("integer %d too big to convert to `signed char'", typeId));
    }

    this.registry.put(extClass, (int) typeId, proc, arg);
    return runtime.getNil();
  }

  @JRubyMethod(required = 2)
  public IRubyObject execute(ThreadContext ctx, IRubyObject data, IRubyObject offset) {
    return executeLimit(ctx, data, offset, null);
  }

  @JRubyMethod(name = "execute_limit", required = 3)
  public IRubyObject executeLimit(ThreadContext ctx, IRubyObject str, IRubyObject off, IRubyObject lim) {
    RubyString input = str.asString();
    int offset = RubyNumeric.fix2int(off);
    int limit = lim == null || lim.isNil() ? -1 : RubyNumeric.fix2int(lim);
    ByteList byteList = input.getByteList();
    if (limit == -1) {
      limit = byteList.length() - offset;
    }
    Decoder decoder = new Decoder(ctx.getRuntime(), this.registry, byteList.unsafeBytes(), byteList.begin() + offset, limit);
    decoder.symbolizeKeys(symbolizeKeys);
    decoder.allowUnknownExt(allowUnknownExt);
    try {
      this.data = null;
      this.data = decoder.next();
    } catch (RaiseException re) {
      if (re.getException().getType() != underflowErrorClass) {
        throw re;
      }
    }
    return ctx.getRuntime().newFixnum(decoder.offset());
  }

  @JRubyMethod(name = "data")
  public IRubyObject getData(ThreadContext ctx) {
    if (data == null) {
      return ctx.getRuntime().getNil();
    } else {
      return data;
    }
  }

  @JRubyMethod(name = "finished?")
  public IRubyObject finished_p(ThreadContext ctx) {
    return data == null ? ctx.getRuntime().getFalse() : ctx.getRuntime().getTrue();
  }

  @JRubyMethod(required = 1)
  public IRubyObject feed(ThreadContext ctx, IRubyObject data) {
    ByteList byteList = data.asString().getByteList();
    if (decoder == null) {
      decoder = new Decoder(ctx.getRuntime(), this.registry, byteList.unsafeBytes(), byteList.begin(), byteList.length());
      decoder.symbolizeKeys(symbolizeKeys);
      decoder.allowUnknownExt(allowUnknownExt);
    } else {
      decoder.feed(byteList.unsafeBytes(), byteList.begin(), byteList.length());
    }
    return this;
  }

  @JRubyMethod(name = "feed_each", required = 1)
  public IRubyObject feedEach(ThreadContext ctx, IRubyObject data, Block block) {
    feed(ctx, data);
    each(ctx, block);
    return ctx.getRuntime().getNil();
  }

  @JRubyMethod
  public IRubyObject each(ThreadContext ctx, Block block) {
    if (block.isGiven()) {
      if (decoder != null) {
        try {
          while (decoder.hasNext()) {
            block.yield(ctx, decoder.next());
          }
        } catch (RaiseException re) {
          if (re.getException().getType() != underflowErrorClass) {
            throw re;
          }
        }
      }
      return this;
    } else {
      return callMethod(ctx, "to_enum");
    }
  }

  @JRubyMethod
  public IRubyObject fill(ThreadContext ctx) {
    return ctx.getRuntime().getNil();
  }

  @JRubyMethod
  public IRubyObject reset(ThreadContext ctx) {
    if (decoder != null) {
      decoder.reset();
    }
    return ctx.getRuntime().getNil();
  }

  @JRubyMethod
  public IRubyObject read(ThreadContext ctx) {
    if (decoder != null) {
      try {
        return decoder.next();
      } catch (RaiseException re) {
        if (re.getException().getType() != underflowErrorClass) {
          throw re;
        } else {
          throw ctx.getRuntime().newEOFError();
        }
      }
    }
    return ctx.getRuntime().getNil();
  }

  @JRubyMethod
  public IRubyObject read_array_header(ThreadContext ctx) {
    if (decoder != null) {
      try {
        return decoder.read_array_header();
      } catch (RaiseException re) {
        if (re.getException().getType() != underflowErrorClass) {
          throw re;
        } else {
          throw ctx.getRuntime().newEOFError();
        }
      }
    }
    return ctx.getRuntime().getNil();
  }

  @JRubyMethod
  public IRubyObject read_map_header(ThreadContext ctx) {
    if (decoder != null) {
      try {
        return decoder.read_map_header();
      } catch (RaiseException re) {
        if (re.getException().getType() != underflowErrorClass) {
          throw re;
        } else {
          throw ctx.getRuntime().newEOFError();
        }
      }
    }
    return ctx.getRuntime().getNil();
  }

  @JRubyMethod(name = "stream")
  public IRubyObject getStream(ThreadContext ctx) {
    if (stream == null) {
      return ctx.getRuntime().getNil();
    } else {
      return stream;
    }
  }

  @JRubyMethod(name = "stream=", required = 1)
  public IRubyObject setStream(ThreadContext ctx, IRubyObject stream) {
    RubyString str;
    if (stream instanceof StringIO) {
      str = stream.callMethod(ctx, "string").asString();
    } else if (stream instanceof RubyIO) {
      str = stream.callMethod(ctx, "read").asString();
    } else {
      throw ctx.getRuntime().newTypeError(stream, "IO");
    }
    ByteList byteList = str.getByteList();
    this.stream = stream;
    this.decoder = null;
    this.decoder = new Decoder(ctx.getRuntime(), this.registry, byteList.unsafeBytes(), byteList.begin(), byteList.length());
    decoder.symbolizeKeys(symbolizeKeys);
    decoder.allowUnknownExt(allowUnknownExt);
    return getStream(ctx);
  }
}
