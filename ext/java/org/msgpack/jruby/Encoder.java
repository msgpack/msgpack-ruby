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
  private final boolean compatibilityMode;

  private ByteBuffer buffer;

  public Encoder(Ruby runtime, boolean compatibilityMode) {
    this.runtime = runtime;
    this.buffer = ByteBuffer.allocate(CACHE_LINE_SIZE - ARRAY_HEADER_SIZE);
    this.binaryEncoding = runtime.getEncodingService().getAscii8bitEncoding();
    this.utf8Encoding = UTF8Encoding.INSTANCE;
    this.compatibilityMode = compatibilityMode;
  }

  private void ensureRemainingCapacity(int c) {
    if (buffer.remaining() < c) {
      int newLength = Math.max(buffer.capacity() + (buffer.capacity() >> 1), buffer.capacity() + c);
      newLength += CACHE_LINE_SIZE - ((ARRAY_HEADER_SIZE + newLength) % CACHE_LINE_SIZE);
      buffer = ByteBuffer.allocate(newLength).put(buffer.array(), 0, buffer.position());
    }
  }

  private IRubyObject readRubyString() {
    IRubyObject str = runtime.newString(new ByteList(buffer.array(), 0, buffer.position(), binaryEncoding, false));
    buffer.clear();
    return str;
  }

  public IRubyObject encode(IRubyObject object) {
    appendObject(object);
    return readRubyString();
  }

  public IRubyObject encode(IRubyObject object, IRubyObject destination) {
    appendObject(object, destination);
    return readRubyString();
  }

  public IRubyObject encodeArrayHeader(int size) {
    appendArrayHeader(size);
    return readRubyString();
  }

  public IRubyObject encodeMapHeader(int size) {
    appendHashHeader(size);
    return readRubyString();
  }

  private void appendObject(IRubyObject object) {
    appendObject(object, null);
  }

  private void appendObject(IRubyObject object, IRubyObject destination) {
    if (object == null || object instanceof RubyNil) {
      ensureRemainingCapacity(1);
      buffer.put(NIL);
    } else if (object instanceof RubyBoolean) {
      ensureRemainingCapacity(1);
      buffer.put(((RubyBoolean) object).isTrue() ? TRUE : FALSE);
    } else if (object instanceof RubyBignum) {
      appendBignum((RubyBignum) object);
    } else if (object instanceof RubyInteger) {
      appendInteger((RubyInteger) object);
    } else if (object instanceof RubyFloat) {
      appendFloat((RubyFloat) object);
    } else if (object instanceof RubyString) {
      appendString((RubyString) object);
    } else if (object instanceof RubySymbol) {
      appendString(((RubySymbol) object).asString());
    } else if (object instanceof RubyArray) {
      appendArray((RubyArray) object);
    } else if (object instanceof RubyHash) {
      appendHash((RubyHash) object);
    } else if (object.respondsTo("to_msgpack")) {
      appendCustom(object, destination);
    } else {
      throw runtime.newArgumentError(String.format("Cannot pack type: %s", object.getClass().getName()));
    }
  }

  private void appendBignum(RubyBignum object) {
    BigInteger value = ((RubyBignum) object).getBigIntegerValue();
    if (value.bitLength() > 64 || (value.bitLength() > 63 && value.signum() < 0)) {
      throw runtime.newArgumentError(String.format("Cannot pack big integer: %s", value));
    }
    ensureRemainingCapacity(9);
    buffer.put(value.signum() < 0 ? INT64 : UINT64);
    byte[] b = value.toByteArray();
    buffer.put(b, b.length - 8, 8);
  }

  private void appendInteger(RubyInteger object) {
    long value = ((RubyInteger) object).getLongValue();
    if (value < 0) {
      if (value < Short.MIN_VALUE) {
        if (value < Integer.MIN_VALUE) {
          ensureRemainingCapacity(9);
          buffer.put(INT64);
          buffer.putLong(value);
        } else {
          ensureRemainingCapacity(5);
          buffer.put(INT32);
          buffer.putInt((int) value);
        }
      } else if (value >= -0x20L) {
        ensureRemainingCapacity(1);
        byte b = (byte) (value | 0xe0);
        buffer.put(b);
      } else if (value < Byte.MIN_VALUE) {
        ensureRemainingCapacity(3);
        buffer.put(INT16);
        buffer.putShort((short) value);
      } else {
        ensureRemainingCapacity(2);
        buffer.put(INT8);
        buffer.put((byte) value);
      }
    } else {
      if (value < 0x10000L) {
        if (value < 128L) {
          ensureRemainingCapacity(1);
          buffer.put((byte) value);
        } else if (value < 0x100L) {
          ensureRemainingCapacity(2);
          buffer.put(UINT8);
          buffer.put((byte) value);
        } else {
          ensureRemainingCapacity(3);
          buffer.put(UINT16);
          buffer.putShort((short) value);
        }
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
  }

  private void appendFloat(RubyFloat object) {
    double value = object.getDoubleValue();
    float f = (float) value;
    //TODO: msgpack-ruby original does encode this value as Double, not float
    // if (Double.compare(f, value) == 0) {
    //   ensureRemainingCapacity(5);
    //   buffer.put(FLOAT32);
    //   buffer.putFloat(f);
    // } else {
      ensureRemainingCapacity(9);
      buffer.put(FLOAT64);
      buffer.putDouble(value);
    // }
  }

  private void appendString(RubyString object) {
    Encoding encoding = object.getEncoding();
    boolean binary = !compatibilityMode && encoding == binaryEncoding;
    if (encoding != utf8Encoding && encoding != binaryEncoding) {
      object = (RubyString) ((RubyString) object).encode(runtime.getCurrentContext(), runtime.getEncodingService().getEncoding(utf8Encoding));
    }
    ByteList bytes = object.getByteList();
    int length = bytes.length();
    if (length < 32 && !binary) {
      ensureRemainingCapacity(1 + length);
      buffer.put((byte) (length | FIXSTR));
    } else if (length <= 0xff && !compatibilityMode) {
      ensureRemainingCapacity(2 + length);
      buffer.put(binary ? BIN8 : STR8);
      buffer.put((byte) length);
    } else if (length <= 0xffff) {
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

  private void appendArray(RubyArray object) {
    appendArrayHeader(object);
    appendArrayElements(object);
  }

  private void appendArrayHeader(RubyArray object) {
    appendArrayHeader(object.size());
  }

  private void appendArrayHeader(int size) {
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
  }

  private void appendArrayElements(RubyArray object) {
    int size = object.size();
    for (int i = 0; i < size; i++) {
      appendObject(object.eltOk(i));
    }
  }

  private void appendHash(RubyHash object) {
    appendHashHeader(object);
    appendHashElements(object);
  }

  private void appendHashHeader(RubyHash object) {
    appendHashHeader(object.size());
  }

  private void appendHashHeader(int size) {
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
  }

  private void appendHashElements(RubyHash object) {
    int size = object.size();
    HashVisitor visitor = new HashVisitor(size);
    object.visitAll(visitor);
    if (visitor.remain != 0) {
      object.getRuntime().newConcurrencyError("Hash size changed while packing");
    }
  }

  private class HashVisitor extends RubyHash.Visitor {
    public int remain;

    public HashVisitor(int size) {
      remain = size;
    }

    public void visit(IRubyObject key, IRubyObject value) {
      if (remain-- > 0) {
        appendObject(key);
        appendObject(value);
      }
    }
  }

  private void appendCustom(IRubyObject object, IRubyObject destination) {
    if (destination == null) {
      IRubyObject result = object.callMethod(runtime.getCurrentContext(), "to_msgpack");
      ByteList bytes = result.asString().getByteList();
      int length = bytes.length();
      ensureRemainingCapacity(length);
      buffer.put(bytes.unsafeBytes(), bytes.begin(), length);
    } else {
      object.callMethod(runtime.getCurrentContext(), "to_msgpack", destination);
    }
  }
}
