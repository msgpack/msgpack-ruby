package org.msgpack.jruby;


import org.jruby.Ruby;
import org.jruby.RubyClass;
import org.jruby.RubyObject;
import org.jruby.RubyHash;
import org.jruby.RubyIO;
import org.jruby.RubyInteger;
import org.jruby.runtime.builtin.IRubyObject;
import org.jruby.anno.JRubyClass;
import org.jruby.anno.JRubyMethod;
import org.jruby.runtime.ThreadContext;
import org.jruby.runtime.ObjectAllocator;
import org.jruby.util.ByteList;


@JRubyClass(name="MessagePack::Packer")
public class Packer extends RubyObject {
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

  @JRubyMethod(name = "write")
  public IRubyObject write(ThreadContext ctx, IRubyObject obj) {
    return buffer.write(ctx, encoder.encode(obj, this));
  }

  @JRubyMethod(name = "write_nil")
  public IRubyObject writeNil(ThreadContext ctx) {
    return write(ctx, null);
  }

  @JRubyMethod(name = "write_array_header")
  public IRubyObject writeArrayHeader(ThreadContext ctx, IRubyObject size) {
    int s = (int) size.convertToInteger().getLongValue();
    return buffer.write(ctx, encoder.encodeArrayHeader(s));
  }

  @JRubyMethod(name = "write_map_header")
  public IRubyObject writeMapHeader(ThreadContext ctx, IRubyObject size) {
    int s = (int) size.convertToInteger().getLongValue();
    return buffer.write(ctx, encoder.encodeMapHeader(s));
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

}
