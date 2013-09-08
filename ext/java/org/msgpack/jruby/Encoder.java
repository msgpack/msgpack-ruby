package org.msgpack.jruby;


import java.math.BigInteger;
import java.nio.ByteBuffer;

import org.jruby.Ruby;
import org.jruby.RubyObject;
import org.jruby.RubyNil;
import org.jruby.RubyBoolean;
import org.jruby.RubyBignum;
import org.jruby.RubyInteger;
import org.jruby.RubyFixnum;
import org.jruby.RubyFloat;
import org.jruby.RubyString;
import org.jruby.RubySymbol;
import org.jruby.RubyArray;
import org.jruby.RubyHash;
import org.jruby.RubyEncoding;
import org.jruby.runtime.builtin.IRubyObject;
import org.jruby.util.ByteList;

import org.jcodings.Encoding;
import org.jcodings.specific.UTF8Encoding;

import static org.msgpack.jruby.Types.*;


public class Encoder {

  private static final int CACHE_LINE_SIZE = 64;
  private static final int ARRAY_HEADER_SIZE = 24;

  private final Ruby runtime;
  private final Encoding binaryEncoding;
  private final Encoding utf8Encoding;

  private ByteBuffer buffer;

  public Encoder(Ruby runtime) {
    this.runtime = runtime;
    this.buffer = ByteBuffer.allocate(CACHE_LINE_SIZE - ARRAY_HEADER_SIZE);
    this.binaryEncoding = runtime.getEncodingService().getAscii8bitEncoding();
    this.utf8Encoding = UTF8Encoding.INSTANCE;
  }

  private void ensureRemainingCapacity(int c) {
    if (buffer.remaining() < c) {
      int newLength = Math.max(buffer.capacity() + (buffer.capacity() >> 1), buffer.capacity() + c);
      newLength += CACHE_LINE_SIZE - ((ARRAY_HEADER_SIZE + newLength) % CACHE_LINE_SIZE);
      buffer = ByteBuffer.allocate(newLength).put(buffer.array(), 0, buffer.position());
    }
  }

  public IRubyObject encode(IRubyObject object) {
    encodeObject(object);
    return runtime.newString(new ByteList(buffer.array(), 0, buffer.position(), binaryEncoding, false));
  }

  private void encodeObject(IRubyObject object) {
    if (object == null || object instanceof RubyNil) {
      ensureRemainingCapacity(1);
      buffer.put(NIL);
    } else if (object instanceof RubyBoolean) {
      ensureRemainingCapacity(1);
      buffer.put(((RubyBoolean) object).isTrue() ? TRUE : FALSE);
    } else if (object instanceof RubyBignum) {
      encodeBignum((RubyBignum) object);
    } else if (object instanceof RubyInteger) {
      encodeInteger((RubyInteger) object);
    } else if (object instanceof RubyFloat) {
      encodeFloat((RubyFloat) object);
    } else if (object instanceof RubyString) {
      encodeString((RubyString) object);
    } else if (object instanceof RubySymbol) {
      encodeString(((RubySymbol) object).asString());
    } else if (object instanceof RubyArray) {
      encodeArray((RubyArray) object);
    } else if (object instanceof RubyHash) {
      encodeHash((RubyHash) object);
    } else if (object.respondsTo("to_msgpack")) {
      appendCustom(object);
    } else {
      throw runtime.newArgumentError(String.format("Cannot pack type: %s", object.getClass().getName()));
    }
  }

  private void encodeBignum(RubyBignum object) {
    BigInteger value = ((RubyBignum) object).getBigIntegerValue();
    if (value.bitLength() > 64 || (value.bitLength() > 63 && value.signum() < 0)) {
      throw runtime.newArgumentError(String.format("Cannot pack big integer: %s", value));
    }
    ensureRemainingCapacity(9);
    buffer.put(value.signum() < 0 ? INT64 : UINT64);
    byte[] b = value.toByteArray();
    buffer.put(b, b.length - 8, 8);
  }

  private void encodeInteger(RubyInteger object) {
    long value = ((RubyInteger) object).getLongValue();
    if (value < Integer.MIN_VALUE) {
      ensureRemainingCapacity(9);
      buffer.put(INT64);
      buffer.putLong(value);
    } else if (value < Short.MIN_VALUE) {
      ensureRemainingCapacity(5);
      buffer.put(INT32);
      buffer.putInt((int) value);
    } else if (value < Byte.MIN_VALUE) {
      ensureRemainingCapacity(3);
      buffer.put(INT16);
      buffer.putShort((short) value);
    } else if (value < -0x20L) {
      ensureRemainingCapacity(2);
      buffer.put(INT8);
      buffer.put((byte) value);
    } else if (value < 0L) {
      ensureRemainingCapacity(1);
      byte b = (byte) (value | 0xe0);
      buffer.put(b);
    } else if (value < 128L) {
      ensureRemainingCapacity(1);
      buffer.put((byte) value);
    } else if (value < 0x100L) {
      ensureRemainingCapacity(2);
      buffer.put(UINT8);
      buffer.put((byte) value);
    } else if (value < 0x10000L) {
      ensureRemainingCapacity(3);
      buffer.put(UINT16);
      buffer.putShort((short) value);
    } else if (value < 0x100000000L) {
      ensureRemainingCapacity(5);
      buffer.put(UINT32);
      buffer.putInt((int) value);
    } else {
      ensureRemainingCapacity(9);
      buffer.put(INT64);
      buffer.putLong(value);
    }
  }

  private void encodeFloat(RubyFloat object) {
    double value = object.getDoubleValue();
    float f = (float) value;
    if (Double.compare(f, value) == 0) {
      ensureRemainingCapacity(5);
      buffer.put(FLOAT32);
      buffer.putFloat(f);
    } else {
      ensureRemainingCapacity(9);
      buffer.put(FLOAT64);
      buffer.putDouble(value);
    }
  }

  private void encodeString(RubyString object) {
    Encoding encoding = object.getEncoding();
    boolean binary = encoding == binaryEncoding;
    if (encoding != utf8Encoding && encoding != binaryEncoding) {
      object = (RubyString) object.encode(runtime.getCurrentContext(), RubyEncoding.newEncoding(runtime, utf8Encoding));
    }
    ByteList bytes = object.getByteList();
    int length = bytes.length();
    if (length < 32 && !binary) {
      ensureRemainingCapacity(1 + length);
      buffer.put((byte) (length | 0xa0));
    } else if (length < 0xff) {
      ensureRemainingCapacity(2 + length);
      buffer.put(binary ? BIN8 : STR8);
      buffer.put((byte) length);
    } else if (length < 0xffff) {
      ensureRemainingCapacity(3 + length);
      buffer.put(binary ? BIN16 : STR16);
      buffer.putShort((short) length);
    } else {
      ensureRemainingCapacity(5 + length);
      buffer.put(binary ? BIN32 : STR32);
      buffer.putInt((int) length);
    }
    buffer.put(bytes.unsafeBytes(), bytes.begin(), length);
  }

  private void encodeArray(RubyArray object) {
    int size = object.size();
    if (size < 16) {
      ensureRemainingCapacity(1);
      buffer.put((byte) (size | 0x90));
    } else if (size < 0x10000) {
      ensureRemainingCapacity(3);
      buffer.put(ARY16);
      buffer.putShort((short) size);
    } else {
      ensureRemainingCapacity(5);
      buffer.put(ARY32);
      buffer.putInt(size);
    }
    for (int i = 0; i < size; i++) {
      encodeObject(object.entry(i));
    }
  }

  private void encodeHash(RubyHash object) {
    int size = object.size();
    if (size < 16) {
      ensureRemainingCapacity(1);
      buffer.put((byte) (size | 0x80));
    } else if (size < 0x10000) {
      ensureRemainingCapacity(3);
      buffer.put(MAP16);
      buffer.putShort((short) size);
    } else {
      ensureRemainingCapacity(5);
      buffer.put(MAP32);
      buffer.putInt(size);
    }
    RubyArray keys = object.keys();
    RubyArray values = object.rb_values();
    for (int i = 0; i < size; i++) {
      encodeObject(keys.entry(i));
      encodeObject(values.entry(i));
    }
  }

  private void appendCustom(IRubyObject object) {
    RubyString string = (RubyString) object.callMethod(runtime.getCurrentContext(), "to_msgpack");
    ByteList bytes = string.getByteList();
    int length = bytes.length();
    ensureRemainingCapacity(length);
    buffer.put(bytes.unsafeBytes(), bytes.begin(), length);
  }
}
