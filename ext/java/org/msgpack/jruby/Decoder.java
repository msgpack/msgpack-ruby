package org.msgpack.jruby;


import java.math.BigInteger;
import java.nio.ByteBuffer;
import java.nio.BufferUnderflowException;
import java.util.Iterator;
import java.util.Arrays;

import org.jruby.Ruby;
import org.jruby.RubyObject;
import org.jruby.RubyClass;
import org.jruby.RubyBignum;
import org.jruby.RubyString;
import org.jruby.RubyHash;
import org.jruby.exceptions.RaiseException;
import org.jruby.runtime.builtin.IRubyObject;
import org.jruby.util.ByteList;

import org.jcodings.Encoding;
import org.jcodings.specific.UTF8Encoding;

import static org.msgpack.jruby.Types.*;


public class Decoder implements Iterator<IRubyObject> {
  private final Ruby runtime;
  private final Encoding binaryEncoding;
  private final Encoding utf8Encoding;
  private final RubyClass unpackErrorClass;
  private final RubyClass underflowErrorClass;

  private ByteBuffer buffer;
  private boolean symbolizeKeys;

  public Decoder(Ruby runtime) {
    this(runtime, new byte[] {}, 0, 0);
  }

  public Decoder(Ruby runtime, byte[] bytes) {
    this(runtime, bytes, 0, bytes.length);
  }

  public Decoder(Ruby runtime, byte[] bytes, int offset, int length) {
    this.runtime = runtime;
    this.binaryEncoding = runtime.getEncodingService().getAscii8bitEncoding();
    this.utf8Encoding = UTF8Encoding.INSTANCE;
    this.unpackErrorClass = runtime.getModule("MessagePack").getClass("UnpackError");
    this.underflowErrorClass = runtime.getModule("MessagePack").getClass("UnderflowError");
    feed(bytes, offset, length);
  }

  public void symbolizeKeys(boolean symbolize) {
    this.symbolizeKeys = symbolize;
  }

  public void feed(byte[] bytes) {
    feed(bytes, 0, bytes.length);
  }

  public void feed(byte[] bytes, int offset, int length) {
    if (buffer == null) {
      buffer = ByteBuffer.wrap(bytes, offset, length);
    } else {
      ByteBuffer newBuffer = ByteBuffer.allocate(buffer.remaining() + length);
      newBuffer.put(buffer);
      newBuffer.put(bytes, offset, length);
      newBuffer.flip();
      buffer = newBuffer;
    }
  }

  public void reset() {
    buffer.rewind();
  }

  public int offset() {
    return buffer.position();
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
    byte[] bytes = readBytes(size);
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
      IRubyObject key = next();
      if (this.symbolizeKeys && key instanceof RubyString) {
          key = ((RubyString) key).intern();
      }
      hash.fastASet(key, next());
    }
    return hash;
  }

  private IRubyObject consumeExtension(int size) {
    int type = buffer.get();
    byte[] payload = readBytes(size);
    return ExtensionValue.newExtensionValue(runtime, type, payload);
  }

  private byte[] readBytes(int size) {
    byte[] payload = new byte[size];
    buffer.get(payload);
    return payload;
  }

  @Override
  public void remove() {
    throw new UnsupportedOperationException();
  }

  @Override
  public boolean hasNext() {
    return buffer.remaining() > 0;
  }

  @Override
  public IRubyObject next() {
    int position = buffer.position();
    try {
      byte b = buffer.get();
      outer: switch ((b >> 4) & 0xf) {
      case 0x8: return consumeHash(b & 0x0f);
      case 0x9: return consumeArray(b & 0x0f);
      case 0xa:
      case 0xb: return consumeString(b & 0x1f, utf8Encoding);
      case 0xc:
        switch (b) {
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
        default: break outer;
        }
      case 0xd:
        switch (b) {
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
        default: break outer;
        }
      case 0xe:
      case 0xf: return runtime.newFixnum((0x1f & b) - 0x20);
      default: return runtime.newFixnum(b);
      }
      buffer.position(position);
      throw runtime.newRaiseException(unpackErrorClass, "Illegal byte sequence");
    } catch (RaiseException re) {
      buffer.position(position);
      throw re;
    } catch (BufferUnderflowException bue) {
      buffer.position(position);
      throw runtime.newRaiseException(underflowErrorClass, "Not enough bytes available");
    }
  }
}
