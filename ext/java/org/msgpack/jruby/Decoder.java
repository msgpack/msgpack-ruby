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

import static org.msgpack.jruby.Types.*;


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

  private IRubyObject consumeExtension(int size) {
    int type = buffer.get();
    byte[] payload = new byte[size];
    buffer.get(payload);
    return ExtensionValue.newExtensionValue(runtime, type, payload);
  }

  public IRubyObject next() {
    int b = buffer.get() & 0xff;
    switch ((byte) b) {
    case NIL:      return runtime.getNil();
    case FALSE:    return runtime.getFalse();
    case TRUE:     return runtime.getTrue();
    case BIN8:     return consumeString(buffer.get() & 0xff, binaryEncoding);
    case BIN16:    return consumeString(buffer.getShort() & 0xffff, binaryEncoding);
    case BIN32:    return consumeString(buffer.getInt(), binaryEncoding);
    case VAREXT8:  return consumeExtension(buffer.get());
    case VAREXT16: return consumeExtension(buffer.getShort());
    case VAREXT32: return consumeExtension(buffer.getInt());
    case FLOAT32:  return runtime.newFloat(buffer.getFloat());
    case FLOAT64:  return runtime.newFloat(buffer.getDouble());
    case UINT8:    return runtime.newFixnum(buffer.get() & 0xffL);
    case UINT16:   return runtime.newFixnum(buffer.getShort() & 0xffffL);
    case UINT32:   return runtime.newFixnum(buffer.getInt() & 0xffffffffL);
    case UINT64:   return consumeUnsignedLong();
    case INT8:     return runtime.newFixnum(buffer.get());
    case INT16:    return runtime.newFixnum(buffer.getShort());
    case INT32:    return runtime.newFixnum(buffer.getInt());
    case INT64:    return runtime.newFixnum(buffer.getLong());
    case FIXEXT1:  return consumeExtension(1);
    case FIXEXT2:  return consumeExtension(2);
    case FIXEXT4:  return consumeExtension(4);
    case FIXEXT8:  return consumeExtension(8);
    case FIXEXT16: return consumeExtension(16);
    case STR8:     return consumeString(buffer.get() & 0xff, utf8Encoding);
    case STR16:    return consumeString(buffer.getShort() & 0xffff, utf8Encoding);
    case STR32:    return consumeString(buffer.getInt(), utf8Encoding);
    case ARY16:    return consumeArray(buffer.getShort() & 0xffff);
    case ARY32:    return consumeArray(buffer.getInt());
    case MAP16:    return consumeHash(buffer.getShort() & 0xffff);
    case MAP32:    return consumeHash(buffer.getInt());
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