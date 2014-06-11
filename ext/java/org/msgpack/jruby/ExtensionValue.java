package org.msgpack.jruby;


import java.util.Arrays;
import java.nio.ByteBuffer;

import org.jruby.Ruby;
import org.jruby.RubyClass;
import org.jruby.RubyObject;
import org.jruby.RubyFixnum;
import org.jruby.RubyString;
import org.jruby.runtime.builtin.IRubyObject;
import org.jruby.runtime.ObjectAllocator;
import org.jruby.runtime.ThreadContext;
import org.jruby.anno.JRubyClass;
import org.jruby.anno.JRubyMethod;
import org.jruby.util.ByteList;

import static org.jruby.runtime.Visibility.PRIVATE;

import org.jcodings.Encoding;

import static org.msgpack.jruby.Types.*;


@JRubyClass(name="MessagePack::ExtensionValue")
public class ExtensionValue extends RubyObject {
  private final Encoding binaryEncoding;

  private RubyFixnum type;
  private RubyString payload;

  public ExtensionValue(Ruby runtime, RubyClass type) {
    super(runtime, type);
    this.binaryEncoding = runtime.getEncodingService().getAscii8bitEncoding();
  }

  public static class ExtensionValueAllocator implements ObjectAllocator {
    public IRubyObject allocate(Ruby runtime, RubyClass klass) {
      return new ExtensionValue(runtime, klass);
    }
  }

  public static ExtensionValue newExtensionValue(Ruby runtime, int type, byte[] payload) {
    ExtensionValue v = new ExtensionValue(runtime, runtime.getModule("MessagePack").getClass("ExtensionValue"));
    ByteList byteList = new ByteList(payload, runtime.getEncodingService().getAscii8bitEncoding());
    v.initialize(runtime.getCurrentContext(), runtime.newFixnum(type), runtime.newString(byteList));
    return v;
  }

  @JRubyMethod(name = "initialize", required = 2, visibility = PRIVATE)
  public IRubyObject initialize(ThreadContext ctx, IRubyObject type, IRubyObject payload) {
    this.type = (RubyFixnum) type;
    this.payload = (RubyString) payload;
    return this;
  }

  @JRubyMethod(name = "to_msgpack")
  public IRubyObject toMsgpack() {
    ByteList payloadBytes = payload.getByteList();
    int payloadSize = payloadBytes.length();
    int outputSize = 0;
    boolean fixSize = payloadSize == 1 || payloadSize == 2 || payloadSize == 4 || payloadSize == 8 || payloadSize == 16;
    if (fixSize) {
      outputSize = 2 + payloadSize;
    } else if (payloadSize < 0x100) {
      outputSize = 3 + payloadSize;
    } else if (payloadSize < 0x10000) {
      outputSize = 4 + payloadSize;
    } else {
      outputSize = 6 + payloadSize;
    }
    byte[] bytes = new byte[outputSize];
    ByteBuffer buffer = ByteBuffer.wrap(bytes);
    if (payloadSize == 1) {
      buffer.put(FIXEXT1);
      buffer.put((byte) type.getLongValue());
      buffer.put((byte) payloadBytes.get(0));
    } else if (payloadSize == 2) {
      buffer.put(FIXEXT2);
      buffer.put((byte) type.getLongValue());
      buffer.put(payloadBytes.unsafeBytes(), payloadBytes.begin(), 2);
    } else if (payloadSize == 4) {
      buffer.put(FIXEXT4);
      buffer.put((byte) type.getLongValue());
      buffer.put(payloadBytes.unsafeBytes(), payloadBytes.begin(), 4);
    } else if (payloadSize == 8) {
      buffer.put(FIXEXT8);
      buffer.put((byte) type.getLongValue());
      buffer.put(payloadBytes.unsafeBytes(), payloadBytes.begin(), 8);
    } else if (payloadSize == 16) {
      buffer.put(FIXEXT16);
      buffer.put((byte) type.getLongValue());
      buffer.put(payloadBytes.unsafeBytes(), payloadBytes.begin(), 16);
    } else if (payloadSize < 0x100) {
      buffer.put(VAREXT8);
      buffer.put((byte) payloadSize);
      buffer.put((byte) type.getLongValue());
      buffer.put(payloadBytes.unsafeBytes(), payloadBytes.begin(), payloadSize);
    } else if (payloadSize < 0x10000) {
      buffer.put(VAREXT16);
      buffer.putShort((short) payloadSize);
      buffer.put((byte) type.getLongValue());
      buffer.put(payloadBytes.unsafeBytes(), payloadBytes.begin(), payloadSize);
    } else {
      buffer.put(VAREXT32);
      buffer.putInt(payloadSize);
      buffer.put((byte) type.getLongValue());
      buffer.put(payloadBytes.unsafeBytes(), payloadBytes.begin(), payloadSize);
    }
    return getRuntime().newString(new ByteList(bytes, binaryEncoding, false));
  }

  @JRubyMethod(name = {"to_s", "inspect"})
  @Override
  public IRubyObject to_s() {
    IRubyObject payloadStr = payload.callMethod(getRuntime().getCurrentContext(), "inspect");
    return getRuntime().newString(String.format("#<MessagePack::ExtensionValue @type=%d, @payload=%s>", type.getLongValue(), payloadStr));
  }

  @JRubyMethod(name = "hash")
  @Override
  public RubyFixnum hash() {
    long hash = payload.hashCode() & (type.getLongValue() << 56);
    return RubyFixnum.newFixnum(getRuntime(), hash);
  }

  @JRubyMethod(name = "eql?")
  public IRubyObject eql_p(ThreadContext ctx, IRubyObject o) {
    if (o instanceof ExtensionValue) {
      ExtensionValue other = (ExtensionValue) o;
      return getRuntime().newBoolean(this.type.callMethod(ctx, "eql?", other.type).isTrue() && this.payload.callMethod(ctx, "eql?", other.payload).isTrue());
    }
    return getRuntime().getFalse();
  }
}