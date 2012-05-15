package msgpack;

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


public class RubyObjectUnpacker {
  private final MessagePack msgPack;

  public RubyObjectUnpacker(MessagePack msgPack) {
    this.msgPack = msgPack;
  }

  public IRubyObject unpack(RubyString str) throws IOException {
    MessagePackBufferUnpacker unpacker = new MessagePackBufferUnpacker(msgPack);
    unpacker.wrap(str.getBytes());
    return valueToRubyObject(str.getRuntime(), unpacker.readValue());
  }

  IRubyObject valueToRubyObject(Ruby runtime, Value value) {
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
      return convert(runtime, value.asArrayValue());
    case MAP:
      return convert(runtime, value.asMapValue());
    case RAW:
      return convert(runtime, value.asRawValue());
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

  private IRubyObject convert(Ruby runtime, ArrayValue value) {
    Value[] elements = value.asArrayValue().getElementArray();
    int elementCount = elements.length;
    IRubyObject[] rubyObjects = new IRubyObject[elementCount];
    for (int i = 0; i < elementCount; i++) {
      rubyObjects[i] = valueToRubyObject(runtime, elements[i]);
    }
    return RubyArray.newArray(runtime, rubyObjects);
  }

  private IRubyObject convert(Ruby runtime, MapValue value) {
    Value[] keysAndValues = value.asMapValue().getKeyValueArray();
    int kvCount = keysAndValues.length;
    RubyHash hash = RubyHash.newHash(runtime);
    for (int i = 0; i < kvCount; i += 2) {
      Value k = keysAndValues[i];
      Value v = keysAndValues[i + 1];
      hash.put(valueToRubyObject(runtime, k), valueToRubyObject(runtime, v));
    }
    return hash;
  }

  private IRubyObject convert(Ruby runtime, RawValue value) {
    return RubyString.newString(runtime, value.asRawValue().getByteArray());
  }
}
