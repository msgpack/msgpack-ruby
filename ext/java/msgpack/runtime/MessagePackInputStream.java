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

import java.io.EOFException;
import java.io.IOException;
import java.io.InputStream;
import java.math.BigInteger;
import java.util.Map;

import msgpack.RubyMessagePack;

import org.jcodings.Encoding;
import org.jcodings.EncodingDB.Entry;
import org.jcodings.specific.USASCIIEncoding;
import org.jcodings.specific.UTF8Encoding;
import org.jruby.Ruby;
import org.jruby.RubyArray;
import org.jruby.RubyBignum;
import org.jruby.RubyClass;
import org.jruby.RubyFixnum;
import org.jruby.RubyFloat;
import org.jruby.RubyHash;
import org.jruby.RubyModule;
import org.jruby.RubyRegexp;
import org.jruby.RubyString;
import org.jruby.javasupport.util.RuntimeHelpers;
import org.jruby.runtime.builtin.IRubyObject;
import org.jruby.runtime.encoding.EncodingCapable;
import org.jruby.runtime.marshal.MarshalStream;
import org.jruby.util.ByteList;
import org.msgpack.MessagePack;
import org.msgpack.MessageTypeException;
import org.msgpack.type.ArrayValue;
import org.msgpack.type.BooleanValue;
import org.msgpack.type.FloatValue;
import org.msgpack.type.IntegerValue;
import org.msgpack.type.MapValue;
import org.msgpack.type.RawValue;
import org.msgpack.type.Value;
import org.msgpack.unpacker.Unpacker;


public class MessagePackInputStream extends InputStream {

    protected final Ruby runtime;

    private Unpacker unpacker;

    private final MessagePackInputCache cache;

    private final IRubyObject proc;

    private final InputStream in;

    private final boolean taint;

    private final boolean untrust;

    public MessagePackInputStream(Ruby runtime, InputStream in, IRubyObject proc, boolean taint, boolean untrust) throws IOException {
        this.runtime = runtime;
        this.unpacker = RubyMessagePack.getMessagePack(runtime).createUnpacker(in);
        this.cache = new MessagePackInputCache(runtime);
        this.proc = proc;
        this.in = in;
        this.taint = taint;
        this.untrust = untrust;
    }

    public InputStream getInputStream() {
	return in;
    }

    public IRubyObject readObject() throws IOException {
        //int type = readUnsignedByte();
	int type = 0;// TODO
        IRubyObject result = null;
        if (cache.isLinkType(type)) {
            result = cache.readLink(this, type);
            if (runtime.is1_9()) return doCallProcForLink(result, type);
        } else {
            result = readObjectDirectly(type);
        }

        result.setTaint(taint);
        result.setUntrusted(untrust);

        return result;
    }

    public void registerLinkTarget(IRubyObject newObject) {
        if (MessagePackOutputStream.shouldBeRegistered(newObject)) {
            cache.register(newObject);
        }
    }

    public static RubyModule getModuleFromPath(Ruby runtime, String path) {
        RubyModule value = runtime.getClassFromPath(path);
        if (!value.isModule()) {
            throw runtime.newArgumentError(path + " does not refer module");
        }
        return value;
    }

    public static RubyClass getClassFromPath(Ruby runtime, String path) {
        RubyModule value = runtime.getClassFromPath(path);
        if (!value.isClass()) {
            throw runtime.newArgumentError(path + " does not refer class");
        }
        return (RubyClass)value;
    }

    private IRubyObject doCallProcForLink(IRubyObject result, int type) {
        if (proc != null && type != ';') {
            // return the result of the proc, but not for symbols
            return RuntimeHelpers.invoke(getRuntime().getCurrentContext(), proc, "call", result);
        }
        return result;
    }

    private IRubyObject doCallProcForObj(IRubyObject result) {
        if (proc != null) {
            // return the result of the proc, but not for symbols
            return RuntimeHelpers.invoke(getRuntime().getCurrentContext(), proc, "call", result);
        }
        return result;
    }

    private IRubyObject readObjectDirectly(int type) throws IOException {
	Value value = unpacker.readValue();
	IRubyObject rubyObj = readObjectDirectly(value);
        if (runtime.is1_9()) {
            return doCallProcForObj(rubyObj);
            // TODO
//        } else if (type != ':') {
//            // call the proc, but not for symbols
//            doCallProcForObj(rubyObj);
        }
	return rubyObj;
    }

    private IRubyObject readObjectDirectly(Value value) throws IOException {
        IRubyObject rubyObj = null;
        if (value.isNilValue()) {
            value.asNilValue();
            rubyObj = runtime.getNil();
        } else if (value.isBooleanValue()) {
            boolean bool = ((BooleanValue) value.asBooleanValue()).getBoolean();
            rubyObj = bool ? runtime.getTrue() : runtime.getFalse();
        } else if (value.isRawValue()){
            byte[] bytes = ((RawValue) value.asRawValue()).getByteArray();
            rubyObj = RubyString.newString(runtime, bytes);
        } else if (value.isIntegerValue()) {
            IntegerValue iv = (IntegerValue) value.asIntegerValue();
            try {
        	long l = iv.getLong();
        	rubyObj = RubyFixnum.newFixnum(runtime, l);
            } catch (MessageTypeException e) {
        	BigInteger bi = iv.getBigInteger();
        	rubyObj = RubyBignum.newBignum(runtime, bi);
            }
        } else if (value.isFloatValue()) {
            double d = ((FloatValue) value.asFloatValue()).getDouble();
            rubyObj = RubyFloat.newFloat(runtime, d);
        } 
        // FIXME RubyRegexp
        // FIXME RubySymbol
        else if (value.isArrayValue()) {
            ArrayValue arrayValue = value.asArrayValue();
            Value[] values = arrayValue.getElementArray();
            int length = values.length;
            RubyArray array = getRuntime().newArray(length);
            registerLinkTarget(array);
            for (int i = 0; i < length; ++i) {
        	IRubyObject element = readObjectDirectly(values[i]);
        	array.append(element);
            }
            rubyObj = (IRubyObject) array;
        } else if (value.isMapValue()) {
            MapValue mapValue = value.asMapValue();
            RubyHash hash = RubyHash.newHash(getRuntime());
            registerLinkTarget(hash);
            for (Map.Entry<Value, Value> e : mapValue.entrySet()) {
        	// TODO
        	hash.fastASetCheckString(getRuntime(),
        		readObjectDirectly(e.getKey()),
        		readObjectDirectly(e.getValue()));
            }
            //if (defaultValue) result.default_value_set(input.unmarshalObject());
            rubyObj = (IRubyObject) hash;
        }
        // FIXME RubyClass
        // FIXME RubyModule
        // FIXME RubySymbol
        // FIXME RubyStruct
        // FIXME defaultObjectUnmarshal
        else {
            throw getRuntime().newArgumentError("dump format error(" + value + ")");
        }

        return rubyObj;
    }


    public Ruby getRuntime() {
        return runtime;
    }

    public int readUnsignedByte() throws IOException {
        int result = read();
        if (result == -1) {
            throw new EOFException("Unexpected end of stream");
        }
        return result;
    }

    public byte readSignedByte() throws IOException {
        int b = readUnsignedByte();
        if (b > 127) {
            return (byte) (b - 256);
        }
        return (byte) b;
    }

    public ByteList unmarshalString() throws IOException {
        int length = unmarshalInt();
        byte[] buffer = new byte[length];

        int readLength = 0;
        while (readLength < length) {
            int read = in.read(buffer, readLength, length - readLength);

            if (read == -1) {
                throw getRuntime().newArgumentError("marshal data too short");
            }
            readLength += read;
        }

        return new ByteList(buffer,false);
    }

    public int unmarshalInt() throws IOException {
        int c = readSignedByte();
        if (c == 0) {
            return 0;
        } else if (5 < c && c < 128) {
            return c - 5;
        } else if (-129 < c && c < -5) {
            return c + 5;
        }
        long result;
        if (c > 0) {
            result = 0;
            for (int i = 0; i < c; i++) {
                result |= (long) readUnsignedByte() << (8 * i);
            }
        } else {
            c = -c;
            result = -1;
            for (int i = 0; i < c; i++) {
                result &= ~((long) 0xff << (8 * i));
                result |= (long) readUnsignedByte() << (8 * i);
            }
        }
        return (int) result;
    }

    public int read() throws IOException {
        return in.read();
    }

    public void close() throws IOException {
	in.close();
    }
}
