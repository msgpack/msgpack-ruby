package org.msgpack.jruby;


import java.math.BigInteger;
import java.nio.ByteBuffer;

import org.jruby.Ruby;
import org.jruby.RubyObject;
import org.jruby.RubyClass;
import org.jruby.RubyBignum;
import org.jruby.RubyHash;
import org.jruby.runtime.builtin.IRubyObject;
import org.jruby.util.ByteList;

import org.jcodings.Encoding;
import org.jcodings.specific.UTF8Encoding;


public class Decoder {
  private final Ruby runtime;
  private final ByteBuffer buffer;
  private final Encoding binaryEncoding;
  private final Encoding utf8Encoding;
  private final RubyClass unpackErrorClass;

  public Decoder(Ruby runtime, byte[] buffer) {
    this.runtime = runtime;
    this.buffer = ByteBuffer.wrap(buffer);
    this.binaryEncoding = runtime.getEncodingService().getAscii8bitEncoding();
    this.utf8Encoding = UTF8Encoding.INSTANCE;
    this.unpackErrorClass = runtime.getModule("MessagePack").getClass("UnpackError");
  }

  private IRubyObject consumeUnsignedLong() {
    long value = buffer.getLong();
    if (value < 0) {
      return RubyBignum.newBignum(runtime, BigInteger.valueOf(value & ((1L<<63)-1)).setBit(63));
    } else {
      return runtime.newFixnum(value);
    }
  }

  private IRubyObject consumeString(int size, Encoding encoding) {
    byte[] bytes = new byte[size];
    buffer.get(bytes);
    ByteList byteList = new ByteList(bytes, encoding);
    return runtime.newString(byteList);
  }

  private IRubyObject consumeArray(int size) {
    IRubyObject[] elements = new IRubyObject[size];
    for (int i = 0; i < size; i++) {
      elements[i] = next();
    }
    return runtime.newArray(elements);
  }

  private IRubyObject consumeHash(int size) {
    RubyHash hash = RubyHash.newHash(runtime);
    for (int i = 0; i < size; i++) {
      hash.fastASet(next(), next());
    }
    return hash;
  }

  public IRubyObject next() {
    int b = buffer.get() & 0xff;
    switch (b) {
    case 0xc0: return runtime.getNil();
    case 0xc2: return runtime.newBoolean(false);
    case 0xc3: return runtime.newBoolean(true);
    case 0xc4: return consumeString(buffer.get() & 0xff, binaryEncoding);
    case 0xc5: return consumeString(buffer.getShort() & 0xffff, binaryEncoding);
    case 0xc6: return consumeString(buffer.getInt(), binaryEncoding);
    case 0xca: return runtime.newFloat(buffer.getFloat());
    case 0xcb: return runtime.newFloat(buffer.getDouble());
    case 0xcc: return runtime.newFixnum(buffer.get() & 0xffL);
    case 0xcd: return runtime.newFixnum(buffer.getShort() & 0xffffL);
    case 0xce: return runtime.newFixnum(buffer.getInt() & 0xffffffffL);
    case 0xcf: return consumeUnsignedLong();
    case 0xd0: return runtime.newFixnum(buffer.get());
    case 0xd1: return runtime.newFixnum(buffer.getShort());
    case 0xd2: return runtime.newFixnum(buffer.getInt());
    case 0xd3: return runtime.newFixnum(buffer.getLong());
    case 0xd9: return consumeString(buffer.get() & 0xff, utf8Encoding);
    case 0xda: return consumeString(buffer.getShort() & 0xffff, utf8Encoding);
    case 0xdb: return consumeString(buffer.getInt(), utf8Encoding);
    case 0xdc: return consumeArray(buffer.getShort() & 0xffff);
    case 0xdd: return consumeArray(buffer.getInt());
    case 0xde: return consumeHash(buffer.getShort() & 0xffff);
    case 0xdf: return consumeHash(buffer.getInt());
    default:
      if (b <= 0x7f) {
        return runtime.newFixnum(b);
      } else if ((b & 0xe0) == 0xe0) {
        return runtime.newFixnum(b - 0x100);
      } else if ((b & 0xe0) == 0xa0) {
        return consumeString(b & 0x1f, utf8Encoding);
      } else if ((b & 0xf0) == 0x90) {
        return consumeArray(b & 0x0f);
      } else if ((b & 0xf0) == 0x80) {
        return consumeHash(b & 0x0f);
      }
    }
    throw runtime.newRaiseException(unpackErrorClass, String.format("Illegal byte sequence"));
  }
}