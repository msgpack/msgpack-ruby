package org.msgpack.jruby;


import org.jruby.Ruby;
import org.jruby.RubyClass;
import org.jruby.RubyObject;
import org.jruby.RubyArray;
import org.jruby.RubyHash;
import org.jruby.RubyIO;
import org.jruby.RubyInteger;
import org.jruby.RubyFixnum;
import org.jruby.runtime.builtin.IRubyObject;
import org.jruby.anno.JRubyClass;
import org.jruby.anno.JRubyMethod;
import org.jruby.runtime.ThreadContext;
import org.jruby.runtime.ObjectAllocator;
import org.jruby.util.ByteList;


@JRubyClass(name="MessagePack::Packer")
public class Packer extends RubyObject {
  public ExtRegistry extRegistry;
  private Buffer buffer;
  private Encoder encoder;

  public Packer(Ruby runtime, RubyClass type) {
    super(runtime, type);
  }

  static class PackerAllocator implements ObjectAllocator {
    public IRubyObject allocate(Ruby runtime, RubyClass type) {
      return new Packer(runtime, type);
    }
  }

  static class ExtRegistry {
    private Ruby runtime;
    public RubyHash hash;
    public RubyHash cache;

    public ExtRegistry(Ruby runtime) {
      this.runtime = runtime;
      hash = RubyHash.newHash(runtime);
      cache = RubyHash.newHash(runtime);
    }

    public ExtRegistry dup() {
      ExtRegistry copy = new ExtRegistry(this.runtime);
      copy.hash = (RubyHash) this.hash.dup(runtime.getCurrentContext());
      copy.cache = RubyHash.newHash(runtime);
      return copy;
    }

    public void put(RubyClass klass, int typeId, IRubyObject proc, IRubyObject arg) {
      RubyArray e = RubyArray.newArray(runtime, new IRubyObject[] { RubyFixnum.newFixnum(runtime, typeId), proc, arg });
      cache.rb_clear();
      hash.fastASet(klass, e);
    }

    // proc, typeId(Fixnum)
    public IRubyObject[] lookup(RubyClass klass) {
      RubyArray e = (RubyArray) hash.fastARef(klass);
      if (e == null) {
        e = (RubyArray) cache.fastARef(klass);
      }
      if (e != null) {
        IRubyObject[] pair = new IRubyObject[] {};
        pair[0] = e.entry(1);
        pair[1] = e.entry(0);
      }

      // check all keys whether it's super class of klass, or not
      /*
      for (IRubyObject keyValue : hash.keys()) {
        RubyClass key = (RubyClass) keyValue;
        // TODO: are there any way to check `key` is a superclass of `klass`?
        if (false) {
          IRubyObject hit = RubyArray.newArray(2); // TODO: value
          cache.fastASet(klass, hit);
          IRubyObject[] pair = new IRubyObject[] {};
          pair[0] = hit.entry(1);
          pair[1] = hit.entry(0);
          return pair;
        }
      }
      */

      return null;
    }
  }

  @JRubyMethod(name = "initialize", optional = 2)
  public IRubyObject initialize(ThreadContext ctx, IRubyObject[] args) {
    boolean compatibilityMode = false;
    if (args.length > 0 && args[args.length - 1] instanceof RubyHash) {
      RubyHash options = (RubyHash) args[args.length - 1];
      compatibilityMode = options.fastARef(ctx.getRuntime().newSymbol("compatibility_mode")).isTrue();
    }
    this.encoder = new Encoder(ctx.getRuntime(), compatibilityMode);
    this.buffer = new Buffer(ctx.getRuntime(), ctx.getRuntime().getModule("MessagePack").getClass("Buffer"));
    this.buffer.initialize(ctx, args);
    return this;
  }

  public void setExtRegistry(ExtRegistry registry) {
    this.extRegistry = registry;
    this.encoder.setRegistry(registry);
  }

  public static Packer newPacker(ThreadContext ctx, ExtRegistry extRegistry, IRubyObject[] args) {
    Packer packer = new Packer(ctx.getRuntime(), ctx.getRuntime().getModule("MessagePack").getClass("Packer"));
    packer.initialize(ctx, args);
    packer.setExtRegistry(extRegistry);
    return packer;
  }

  //TODO: registered_types_internal
  //TODO: register_type

  @JRubyMethod(name = "write")
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

  @JRubyMethod(name = "to_s")
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
