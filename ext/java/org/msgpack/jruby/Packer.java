package org.msgpack.jruby;


import org.jruby.Ruby;
import org.jruby.RubyClass;
import org.jruby.RubyObject;
import org.jruby.RubyArray;
import org.jruby.RubyHash;
import org.jruby.RubyIO;
import org.jruby.RubyInteger;
import org.jruby.RubyFixnum;
import org.jruby.runtime.Block;
import org.jruby.runtime.builtin.IRubyObject;
import org.jruby.anno.JRubyClass;
import org.jruby.anno.JRubyMethod;
import org.jruby.runtime.ThreadContext;
import org.jruby.runtime.ObjectAllocator;
import org.jruby.util.ByteList;

import static org.jruby.runtime.Visibility.PRIVATE;

@JRubyClass(name="MessagePack::Packer")
public class Packer extends RubyObject {
  public ExtensionRegistry registry;
  private Buffer buffer;
  private Encoder encoder;

  public Packer(Ruby runtime, RubyClass type, ExtensionRegistry registry) {
    super(runtime, type);
    this.registry = registry;
  }

  static class PackerAllocator implements ObjectAllocator {
    public IRubyObject allocate(Ruby runtime, RubyClass type) {
      return new Packer(runtime, type, null);
    }
  }

  static class ExtensionRegistry {
    private Ruby runtime;
    public RubyHash hash;
    public RubyHash cache;

    public ExtensionRegistry(Ruby runtime) {
      this.runtime = runtime;
      hash = RubyHash.newHash(runtime);
      cache = RubyHash.newHash(runtime);
    }

    public ExtensionRegistry dup() {
      ExtensionRegistry copy = new ExtensionRegistry(runtime);
      copy.hash = (RubyHash) hash.dup(runtime.getCurrentContext());
      copy.cache = RubyHash.newHash(runtime);
      return copy;
    }

    public void put(RubyClass klass, int typeId, IRubyObject proc, IRubyObject arg) {
      RubyArray e = RubyArray.newArray(runtime, new IRubyObject[] { RubyFixnum.newFixnum(runtime, typeId), proc, arg });
      cache.rb_clear();
      hash.fastASet(klass, e);
    }

    public IRubyObject[] lookup(final RubyClass klass) {
      RubyArray e = (RubyArray) hash.fastARef(klass);
      if (e == null) {
        e = (RubyArray) cache.fastARef(klass);
      }
      if (e != null) {
        IRubyObject[] hit = new IRubyObject[2];
        hit[0] = e.entry(1);
        hit[1] = e.entry(0);
        return hit;
      }

      final IRubyObject[] pair = new IRubyObject[2];
      // check all keys whether it's super class of klass, or not
      hash.visitAll(new RubyHash.Visitor() {
        public void visit(IRubyObject keyValue, IRubyObject value) {
          if (pair[0] != null) {
            return;
          }
          ThreadContext ctx = runtime.getCurrentContext();
          RubyArray ancestors = (RubyArray) klass.callMethod(ctx, "ancestors");
          if (ancestors.callMethod(ctx, "include?", keyValue).isTrue()) {
            RubyArray hit = (RubyArray) hash.fastARef(keyValue);
            cache.fastASet(klass, hit);
            pair[0] = hit.entry(1);
            pair[1] = hit.entry(0);
          }
        }
      });

      if (pair[0] == null) {
        return null;
      }
      return pair;
    }
  }

  @JRubyMethod(name = "initialize", optional = 2)
  public IRubyObject initialize(ThreadContext ctx, IRubyObject[] args) {
    boolean compatibilityMode = false;
    if (args.length > 0 && args[args.length - 1] instanceof RubyHash) {
      RubyHash options = (RubyHash) args[args.length - 1];
      IRubyObject mode = options.fastARef(ctx.getRuntime().newSymbol("compatibility_mode"));
      compatibilityMode = (mode != null) && mode.isTrue();
    }
    this.encoder = new Encoder(ctx.getRuntime(), compatibilityMode, registry);
    this.buffer = new Buffer(ctx.getRuntime(), ctx.getRuntime().getModule("MessagePack").getClass("Buffer"));
    this.buffer.initialize(ctx, args);
    this.registry = new ExtensionRegistry(ctx.getRuntime());
    return this;
  }

  public static Packer newPacker(ThreadContext ctx, ExtensionRegistry extRegistry, IRubyObject[] args) {
    Packer packer = new Packer(ctx.getRuntime(), ctx.getRuntime().getModule("MessagePack").getClass("Packer"), extRegistry);
    packer.initialize(ctx, args);
    return packer;
  }

  @JRubyMethod(name = "compatibility_mode?")
  public IRubyObject isCompatibilityMode(ThreadContext ctx) {
    return encoder.isCompatibilityMode() ? ctx.getRuntime().getTrue() : ctx.getRuntime().getFalse();
  }

  @JRubyMethod(name = "registered_types_internal", visibility = PRIVATE)
  public IRubyObject registeredTypesInternal(ThreadContext ctx) {
    return registry.hash.dup(ctx);
  }

  @JRubyMethod(name = "register_type", required = 2, optional = 1)
  public IRubyObject registerType(ThreadContext ctx, IRubyObject[] args, final Block block) {
    // register_type(type, Class){|obj| how_to_serialize.... }
    // register_type(type, Class, :to_msgpack_ext)
    Ruby runtime = ctx.getRuntime();
    IRubyObject type = args[0];
    IRubyObject klass = args[1];

    IRubyObject arg;
    IRubyObject proc;
    if (args.length == 2) {
      if (! block.isGiven()) {
        throw runtime.newLocalJumpErrorNoBlock();
      }
      proc = block.getProcObject();
      arg = proc;
    } else if (args.length == 3) {
      arg = args[2];
      proc = arg.callMethod(ctx, "to_proc");
    } else {
      throw runtime.newArgumentError(String.format("wrong number of arguments (%d for 2..3)", 2 + args.length));
    }

    long typeId = ((RubyFixnum) type).getLongValue();
    if (typeId < -128 || typeId > 127) {
      throw runtime.newRangeError(String.format("integer %d too big to convert to `signed char'", typeId));
    }

    if (!(klass instanceof RubyClass)) {
      throw runtime.newArgumentError(String.format("expected Class but found %s.", klass.getType().getName()));
    }
    RubyClass extClass = (RubyClass) klass;

    registry.put(extClass, (int) typeId, proc, arg);
    return runtime.getNil();
  }

  @JRubyMethod(name = "write", alias = { "pack" })
  public IRubyObject write(ThreadContext ctx, IRubyObject obj) {
    buffer.write(ctx, encoder.encode(obj, this));
    return this;
  }

  @JRubyMethod(name = "write_nil")
  public IRubyObject writeNil(ThreadContext ctx) {
    write(ctx, null);
    return this;
  }

  @JRubyMethod(name = "write_array_header")
  public IRubyObject writeArrayHeader(ThreadContext ctx, IRubyObject size) {
    int s = (int) size.convertToInteger().getLongValue();
    buffer.write(ctx, encoder.encodeArrayHeader(s));
    return this;
  }

  @JRubyMethod(name = "write_map_header")
  public IRubyObject writeMapHeader(ThreadContext ctx, IRubyObject size) {
    int s = (int) size.convertToInteger().getLongValue();
    buffer.write(ctx, encoder.encodeMapHeader(s));
    return this;
  }

  @JRubyMethod(name = "to_s", alias = { "to_str" })
  public IRubyObject toS(ThreadContext ctx) {
    return buffer.toS(ctx);
  }

  @JRubyMethod(name = "buffer")
  public IRubyObject buffer(ThreadContext ctx) {
    return buffer;
  }

  @JRubyMethod(name = "flush")
  public IRubyObject flush(ThreadContext ctx) {
    return buffer.flush(ctx);
  }

  @JRubyMethod(name = "size")
  public IRubyObject size(ThreadContext ctx) {
    return buffer.size(ctx);
  }

  @JRubyMethod(name = "clear")
  public IRubyObject clear(ThreadContext ctx) {
    return buffer.clear(ctx);
  }
}
