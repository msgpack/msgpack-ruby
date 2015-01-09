package org.msgpack.jruby;


import org.jruby.Ruby;
import org.jruby.RubyClass;
import org.jruby.RubyString;
import org.jruby.RubyObject;
import org.jruby.RubyHash;
import org.jruby.RubyNumeric;
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
  private IRubyObject stream;
  private IRubyObject data;
  private Decoder decoder;
  private final RubyClass underflowErrorClass;

  public Unpacker(Ruby runtime, RubyClass type) {
    super(runtime, type);
    this.underflowErrorClass = runtime.getModule("MessagePack").getClass("UnderflowError");
  }

  static class UnpackerAllocator implements ObjectAllocator {
    public IRubyObject allocate(Ruby runtime, RubyClass klass) {
      return new Unpacker(runtime, klass);
    }
  }

  @JRubyMethod(name = "initialize", optional = 1, visibility = PRIVATE)
  public IRubyObject initialize(ThreadContext ctx, IRubyObject[] args) {
    if (args.length > 0) {
      if (args[args.length - 1] instanceof RubyHash) {
        //TODO: symbolize_keys
      } else if (!(args[0] instanceof RubyHash)) {
        setStream(ctx, args[0]);
      }
    }
    return this;
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
    Decoder decoder = new Decoder(ctx.getRuntime(), byteList.unsafeBytes(), byteList.begin() + offset, limit);
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
      decoder = new Decoder(ctx.getRuntime(), byteList.unsafeBytes(), byteList.begin(), byteList.length());
    } else {
      decoder.feed(byteList.unsafeBytes(), byteList.begin(), byteList.length());
    }
    return ctx.getRuntime().getNil();
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
    this.decoder = new Decoder(ctx.getRuntime(), byteList.unsafeBytes(), byteList.begin(), byteList.length());
    return getStream(ctx);
  }
}
