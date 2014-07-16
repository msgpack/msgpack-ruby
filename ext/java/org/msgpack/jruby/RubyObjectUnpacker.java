package org.msgpack.jruby;


import java.io.IOException;

import org.msgpack.MessagePack;
import org.msgpack.MessageTypeException;
import org.msgpack.unpacker.MessagePackBufferUnpacker;
import org.msgpack.type.Value;
import org.msgpack.type.ValueType;
import org.msgpack.type.BooleanValue;
import org.msgpack.type.IntegerValue;
import org.msgpack.type.FloatValue;
import org.msgpack.type.ArrayValue;
import org.msgpack.type.MapValue;
import org.msgpack.type.RawValue;

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
import org.jruby.runtime.builtin.IRubyObject;
import org.jruby.runtime.encoding.EncodingService;
import org.jruby.runtime.ThreadContext;

import org.jcodings.Encoding;


public class RubyObjectUnpacker {
  private final MessagePack msgPack;

  public RubyObjectUnpacker(MessagePack msgPack) {
    this.msgPack = msgPack;
  }

  static class CompiledOptions {
    public final boolean symbolizeKeys;
    public final Encoding encoding;

    public CompiledOptions(Ruby runtime) {
      this(runtime, null);
    }

    public CompiledOptions(Ruby runtime, RubyHash options) {
      EncodingService encodingService = runtime.getEncodingService();
      Encoding externalEncoding = null;
      if (options == null) {
        symbolizeKeys = false;
      } else {
        ThreadContext ctx = runtime.getCurrentContext();
        RubySymbol key = runtime.newSymbol("symbolize_keys");
        IRubyObject value = options.fastARef(key);
        symbolizeKeys = value != null && value.isTrue();
        IRubyObject rubyEncoding = options.fastARef(runtime.newSymbol("encoding"));
        externalEncoding = encodingService.getEncodingFromObject(rubyEncoding);
      }
      encoding = (externalEncoding != null) ? externalEncoding : runtime.getDefaultExternalEncoding();
    }
  }

  public IRubyObject unpack(RubyString str, RubyHash options) throws IOException {
    return unpack(str.getRuntime(), str.getBytes(), new CompiledOptions(str.getRuntime(), options));
  }

  public IRubyObject unpack(Ruby runtime, byte[] data) throws IOException {
    return unpack(runtime, data, new CompiledOptions(runtime, null));
  }

  public IRubyObject unpack(Ruby runtime, byte[] data, RubyHash options) throws IOException {
    return unpack(runtime, data, new CompiledOptions(runtime, options));
  }

  IRubyObject unpack(Ruby runtime, byte[] data, CompiledOptions options) throws IOException {
    MessagePackBufferUnpacker unpacker = new MessagePackBufferUnpacker(msgPack);
    unpacker.wrap(data);
    return valueToRubyObject(runtime, unpacker.readValue(), options);
  }

  IRubyObject valueToRubyObject(Ruby runtime, Value value, RubyHash options) throws IOException {
    return valueToRubyObject(runtime, value, new CompiledOptions(runtime, options));
  }

  IRubyObject valueToRubyObject(Ruby runtime, Value value, CompiledOptions options) {
    switch (value.getType()) {
    case NIL:
      return runtime.getNil();
    case BOOLEAN:
      return convert(runtime, value.asBooleanValue());
    case INTEGER:
      return convert(runtime, value.asIntegerValue());
    case FLOAT:
      return convert(runtime, value.asFloatValue());
    case ARRAY:
      return convert(runtime, value.asArrayValue(), options);
    case MAP:
      return convert(runtime, value.asMapValue(), options);
    case RAW:
      return convert(runtime, value.asRawValue(), options);
    default:
      throw runtime.newArgumentError(String.format("Unexpected value: %s", value.toString()));
    }
  }

  private IRubyObject convert(Ruby runtime, BooleanValue value) {
    return RubyBoolean.newBoolean(runtime, value.asBooleanValue().getBoolean());
  }

  private IRubyObject convert(Ruby runtime, IntegerValue value) {
    // TODO: is there any way of checking for bignums up front?
    IntegerValue iv = value.asIntegerValue();
    try {
      return RubyFixnum.newFixnum(runtime, iv.getLong());
    } catch (MessageTypeException mte) {
      return RubyBignum.newBignum(runtime, iv.getBigInteger());
    }
  }

  private IRubyObject convert(Ruby runtime, FloatValue value) {
    return RubyFloat.newFloat(runtime, value.asFloatValue().getDouble());
  }

  private IRubyObject convert(Ruby runtime, ArrayValue value, CompiledOptions options) {
    Value[] elements = value.asArrayValue().getElementArray();
    int elementCount = elements.length;
    IRubyObject[] rubyObjects = new IRubyObject[elementCount];
    for (int i = 0; i < elementCount; i++) {
      rubyObjects[i] = valueToRubyObject(runtime, elements[i], options);
    }
    return RubyArray.newArray(runtime, rubyObjects);
  }

  private IRubyObject convert(Ruby runtime, MapValue value, CompiledOptions options) {
    Value[] keysAndValues = value.asMapValue().getKeyValueArray();
    int kvCount = keysAndValues.length;
    RubyHash hash = RubyHash.newHash(runtime);
    for (int i = 0; i < kvCount; i += 2) {
      Value k = keysAndValues[i];
      Value v = keysAndValues[i + 1];
      IRubyObject kk = valueToRubyObject(runtime, k, options);
      IRubyObject vv = valueToRubyObject(runtime, v, options);
      if (options.symbolizeKeys) {
        kk = runtime.newSymbol(kk.asString().getByteList());
      }
      hash.put(kk, vv);
    }
    return hash;
  }

  private IRubyObject convert(Ruby runtime, RawValue value, CompiledOptions options) {
    RubyString string = RubyString.newStringNoCopy(runtime, value.getByteArray());
    string.setEncoding(options.encoding);
    string.callMethod(runtime.getCurrentContext(), "encode!");
    return string;
  }
}
