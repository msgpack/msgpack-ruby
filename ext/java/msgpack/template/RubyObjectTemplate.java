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
package msgpack.template;

import java.io.IOException;
import java.math.BigInteger;
import java.util.Iterator;
import java.util.Map;

import org.jruby.Ruby;
import org.jruby.RubyArray;
import org.jruby.RubyBignum;
import org.jruby.RubyFixnum;
import org.jruby.RubyFloat;
import org.jruby.RubyHash;
import org.jruby.RubyString;
import org.jruby.runtime.ClassIndex;
import org.jruby.runtime.builtin.IRubyObject;
import org.jruby.runtime.marshal.CoreObjectType;
import org.jruby.runtime.marshal.DataType;
import org.msgpack.MessageTypeException;
import org.msgpack.packer.BufferPacker;
import org.msgpack.packer.Packer;
import org.msgpack.template.AbstractTemplate;
import org.msgpack.template.Templates;
import org.msgpack.type.ArrayValue;
import org.msgpack.type.BooleanValue;
import org.msgpack.type.FloatValue;
import org.msgpack.type.IntegerValue;
import org.msgpack.type.MapValue;
import org.msgpack.type.RawValue;
import org.msgpack.type.Value;
import org.msgpack.unpacker.Unpacker;


public class RubyObjectTemplate extends AbstractTemplate<IRubyObject> {

    private final Ruby runtime;

    private int depth = 0;

    public RubyObjectTemplate(Ruby runtime) throws IOException {
        this.runtime = runtime;
    }

    public void write(Packer packer, IRubyObject value, boolean required) throws IOException {
	writeRubyObject(packer, value);
    }

    private void writeRubyObject(Packer packer, IRubyObject value) throws IOException {
        depth++;
        writeRubyObjectDirectly(packer, value);
        depth--;
        if (depth == 0) {
            packer.flush();
        }
    }

    private void writeRubyObjectDirectly(Packer packer, IRubyObject value) throws IOException {
        if (value instanceof CoreObjectType) {
            if (value instanceof DataType) {
        	// FIXME #MN
                throw value.getRuntime().newTypeError(
                	"no marshal_dump is defined for class " + value.getMetaClass().getName());
            }
            int nativeTypeIndex = ((CoreObjectType) value).getNativeTypeIndex();
            switch (nativeTypeIndex) {
            case ClassIndex.NIL:
        	writeNil(packer);
                return;
            case ClassIndex.STRING:
        	writeString(packer, (RubyString) value);
                return;
            case ClassIndex.TRUE:
        	writeTrue(packer);
                return;
            case ClassIndex.FALSE:
        	writeFalse(packer);
                return;
            case ClassIndex.FIXNUM:
        	writeFixnum(packer, (RubyFixnum) value);
        	return;
            case ClassIndex.FLOAT:
        	writeFloat(packer, (RubyFloat) value);
                return;
            case ClassIndex.BIGNUM:
        	writeBignum(packer, (RubyBignum) value);
                return;
            case ClassIndex.ARRAY:
                writeArray(packer, (RubyArray) value);
                return;
            case ClassIndex.HASH:
                writeHash(packer, (RubyHash) value);
                return;
            case ClassIndex.CLASS:
        	throw runtime.newNotImplementedError("class index"); // TODO #MN
            case ClassIndex.MODULE:
        	throw runtime.newNotImplementedError("module index"); // TODO #MN
            case ClassIndex.OBJECT:
            case ClassIndex.BASICOBJECT:
        	throw runtime.newNotImplementedError("object or basicobject index"); // TODO #MN
            case ClassIndex.REGEXP:
        	throw runtime.newNotImplementedError("regexp index"); // TODO #MN
            case ClassIndex.STRUCT:
        	throw runtime.newNotImplementedError("struct index"); // TODO #MN
            case ClassIndex.SYMBOL:
        	throw runtime.newNotImplementedError("symbol index"); // TODO #MN
            default:
        	throw runtime.newTypeError("can't pack " + value.getMetaClass().getName());
            }
        } else {
            throw runtime.newTypeError("can't pack " + value.getMetaClass().getName()); // TODO #MN
        }
    }

    private void writeNil(Packer packer) throws IOException {
	packer.writeNil();
    }

    private void writeString(Packer packer, RubyString string) throws IOException {
	Templates.TString.write(packer, string.asJavaString());
    }

    private void writeTrue(Packer packer) throws IOException {
	Templates.TBoolean.write(packer, true);
    }

    private void writeFalse(Packer packer) throws IOException {
	Templates.TBoolean.write(packer, false);
    }

    private void writeFixnum(Packer packer, RubyFixnum fixnum) throws IOException {
	Templates.TLong.write(packer, fixnum.getLongValue());
    }

    private void writeFloat(Packer packer, RubyFloat f) throws IOException {
	Templates.TDouble.write(packer, f.getValue());
    }

    private void writeBignum(Packer packer, RubyBignum bignum) throws IOException {
	Templates.TBigInteger.write(packer, bignum.getValue());
    }

    private void writeArray(Packer packer, RubyArray array) throws IOException {
        int length = array.getLength();
        packer.writeArrayBegin(length);
        for (int i = 0; i < length; ++i) {
            writeRubyObject(packer, array.eltInternal(i));
        }
        packer.writeArrayEnd();
    }

    @SuppressWarnings({ "rawtypes" })
    private void writeHash(Packer packer, RubyHash hash) throws IOException {
	int size = hash.size();
	packer.writeMapBegin(size);
	Iterator iter = hash.directEntrySet().iterator();
	while (iter.hasNext()) {
	    Map.Entry e = (Map.Entry) iter.next();
	    writeRubyObject(packer, (IRubyObject) e.getKey());
	    writeRubyObject(packer, (IRubyObject) e.getValue());
	}
	packer.writeMapEnd();
    }

    public IRubyObject read(Unpacker u, IRubyObject to, boolean required) throws IOException {
	return readRubyObjectDirectly(u);
    }

    public IRubyObject readRubyObjectDirectly(Unpacker unpacker) throws IOException {
	Value value = unpacker.readValue();
	return readRubyObjectDirectly(unpacker, value);
    }

    private IRubyObject readRubyObjectDirectly(Unpacker unpacker, Value value) throws IOException {
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

    private IRubyObject readNil(Unpacker unpacker, Value value) throws IOException {
	value.asNilValue();
	return runtime.getNil();
    }

    private IRubyObject readBoolean(Unpacker unpacker, Value value) throws IOException {
	boolean bool = ((BooleanValue) value.asBooleanValue()).getBoolean();
	return bool ? runtime.getTrue() : runtime.getFalse();
    }

    private IRubyObject readRaw(Unpacker unpacker, Value value) throws IOException {
        byte[] bytes = ((RawValue) value.asRawValue()).getByteArray();
        return RubyString.newString(runtime, bytes);
    }

    private IRubyObject readInteger(Unpacker unpacker, Value value) throws IOException {
	IntegerValue iv = (IntegerValue) value.asIntegerValue();
	try {
	    long l = iv.getLong();
	    return RubyFixnum.newFixnum(runtime, l);
	} catch (MessageTypeException e) {
	    BigInteger bi = iv.getBigInteger();
	    return RubyBignum.newBignum(runtime, bi);
	}
    }

    private IRubyObject readFloat(Unpacker unpacker, Value value) throws IOException {
	double d = ((FloatValue) value.asFloatValue()).getDouble();
        return RubyFloat.newFloat(runtime, d);
    }

    private IRubyObject readArray(Unpacker unpacker, Value value) throws IOException {
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

    private IRubyObject readHash(Unpacker unpacker, Value value) throws IOException {
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
