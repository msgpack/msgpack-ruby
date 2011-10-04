//
// MessagePack for Ruby
//
// Copyright (C) 2008-2011 FURUHASHI Sadayuki
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.
//
package msgpack.runtime;

import java.io.IOException;
import java.math.BigInteger;
import java.util.Map;

import org.jruby.Ruby;
import org.jruby.RubyArray;
import org.jruby.RubyBignum;
import org.jruby.RubyFixnum;
import org.jruby.RubyFloat;
import org.jruby.RubyHash;
import org.jruby.RubyString;
import org.jruby.runtime.builtin.IRubyObject;
import org.msgpack.MessageTypeException;
import org.msgpack.type.ArrayValue;
import org.msgpack.type.BooleanValue;
import org.msgpack.type.FloatValue;
import org.msgpack.type.IntegerValue;
import org.msgpack.type.MapValue;
import org.msgpack.type.RawValue;
import org.msgpack.type.Value;
import org.msgpack.unpacker.BufferUnpacker;


public class RubyObjectUnpacker {

    protected final Ruby runtime;

    public RubyObjectUnpacker(Ruby runtime) throws IOException {
        this.runtime = runtime;
    }

    public IRubyObject readRubyObject(BufferUnpacker unpacker,
	    byte[] bytes, int offset, int length, boolean taint, boolean untrust) throws IOException {
	unpacker.wrap(bytes, offset, length);
	IRubyObject result = readRubyObjectDirectly(unpacker);
        result.setTaint(taint);
        result.setUntrusted(untrust);
        return result;
    }

    public IRubyObject readRubyObjectDirectly(BufferUnpacker unpacker) throws IOException {
	Value value = unpacker.readValue();
	return readRubyObjectDirectly(unpacker, value);
    }

    private IRubyObject readRubyObjectDirectly(BufferUnpacker unpacker, Value value) throws IOException {
        if (value.isNilValue()) {
            return readNil(unpacker, value);
        } else if (value.isBooleanValue()) {
            return readBoolean(unpacker, value);
        } else if (value.isRawValue()){
            return readRaw(unpacker, value);
        } else if (value.isIntegerValue()) {
            return readInteger(unpacker, value);
        } else if (value.isFloatValue()) {
            return readFloat(unpacker, value);
        } else if (value.isArrayValue()) { 
            return readArray(unpacker, value);
        } else if (value.isMapValue()) {
            return readHash(unpacker, value);
        }
        // FIXME RubyRegexp
        // FIXME RubySymbol
        // FIXME RubyClass
        // FIXME RubyModule
        // FIXME RubySymbol
        // FIXME RubyStruct
        // FIXME defaultObjectUnmarshal
        else {
            throw runtime.newArgumentError("dump format error(" + value + ")");
        }
    }

    private IRubyObject readNil(BufferUnpacker unpacker, Value value) throws IOException {
	value.asNilValue();
	return runtime.getNil();
    }

    private IRubyObject readBoolean(BufferUnpacker unpacker, Value value) throws IOException {
	boolean bool = ((BooleanValue) value.asBooleanValue()).getBoolean();
	return bool ? runtime.getTrue() : runtime.getFalse();
    }

    private IRubyObject readRaw(BufferUnpacker unpacker, Value value) throws IOException {
        byte[] bytes = ((RawValue) value.asRawValue()).getByteArray();
        return RubyString.newString(runtime, bytes);
    }

    private IRubyObject readInteger(BufferUnpacker unpacker, Value value) throws IOException {
	IntegerValue iv = (IntegerValue) value.asIntegerValue();
	try {
	    long l = iv.getLong();
	    return RubyFixnum.newFixnum(runtime, l);
	} catch (MessageTypeException e) {
	    BigInteger bi = iv.getBigInteger();
	    return RubyBignum.newBignum(runtime, bi);
	}
    }

    private IRubyObject readFloat(BufferUnpacker unpacker, Value value) throws IOException {
	double d = ((FloatValue) value.asFloatValue()).getDouble();
        return RubyFloat.newFloat(runtime, d);
    }

    private IRubyObject readArray(BufferUnpacker unpacker, Value value) throws IOException {
	ArrayValue arrayValue = value.asArrayValue();
	Value[] values = arrayValue.getElementArray();
	int length = values.length;
	RubyArray array = runtime.newArray(length);
	for (int i = 0; i < length; ++i) {
	    IRubyObject element = readRubyObjectDirectly(unpacker, values[i]);
	    array.append(element);
	}
	return (IRubyObject) array;
    }

    private IRubyObject readHash(BufferUnpacker unpacker, Value value) throws IOException {
	MapValue mapValue = value.asMapValue();
        RubyHash hash = RubyHash.newHash(runtime);
        for (Map.Entry<Value, Value> e : mapValue.entrySet()) {
    	// TODO
    	hash.fastASetCheckString(runtime,
    		readRubyObjectDirectly(unpacker, e.getKey()),
    		readRubyObjectDirectly(unpacker, e.getValue()));
        }
        //if (defaultValue) result.default_value_set(input.unmarshalObject());
        return (IRubyObject) hash;
    }
}
