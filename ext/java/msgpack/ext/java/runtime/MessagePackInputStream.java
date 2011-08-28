package msgpack.ext.java.runtime;

import java.io.EOFException;
import java.io.IOException;
import java.io.InputStream;
import java.math.BigInteger;
import java.util.Map;

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
import org.jruby.RubyObject;
import org.jruby.RubyRegexp;
import org.jruby.RubyString;
import org.jruby.RubySymbol;
import org.jruby.exceptions.RaiseException;
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

    // introduced for keeping ivar read state recursively.
    private class MessagePackState {
        private boolean ivarWaiting;
        
        MessagePackState(boolean ivarWaiting) {
            this.ivarWaiting = ivarWaiting;
        }
        
        boolean isIvarWaiting() {
            return ivarWaiting;
        }
        
        void setIvarWaiting(boolean ivarWaiting) {
            this.ivarWaiting = ivarWaiting;
        }
    }

    protected final Ruby runtime;

    private Unpacker unpacker;

    private final MessagePackInputCache cache;

    private final IRubyObject proc;

    private final InputStream inputStream;

    private final boolean taint;

    private final boolean untrust;

    public MessagePackInputStream(Ruby runtime, MessagePack msgpack, InputStream in, IRubyObject proc, boolean taint, boolean untrust) throws IOException {
        this.runtime = runtime;
        this.unpacker = msgpack.createUnpacker(in);
        this.cache = new MessagePackInputCache(runtime);
        this.proc = proc;
        this.inputStream = in;
        this.taint = taint;
        this.untrust = untrust;
    }

    // r_object
    public IRubyObject readObject() throws IOException {
        return readObject(new MessagePackState(false), true);
    }

    // r_object
    public IRubyObject readObject(boolean callProc) throws IOException {
        return readObject(new MessagePackState(false), callProc);
    }

    // r_object0
    public IRubyObject readObject(MessagePackState state, boolean callProc) throws IOException {
        //int type = readUnsignedByte();
	int type = 0;// TODO
        IRubyObject result = null;
        if (cache.isLinkType(type)) {
            result = cache.readLink(this, type);
            if (callProc && runtime.is1_9()) return doCallProcForLink(result, type);
        } else {
            result = readObjectDirectly(type, state, callProc);
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

    private IRubyObject readObjectDirectly(int type, MessagePackState state, boolean callProc) throws IOException {
	Value value = unpacker.readValue();
	IRubyObject rubyObj = readObjectDirectly(value, state, callProc);
        if (runtime.is1_9()) {
            if (callProc) {
                return doCallProcForObj(rubyObj);
            }
            // TODO
//        } else if (type != ':') {
//            // call the proc, but not for symbols
//            doCallProcForObj(rubyObj);
        }
	return rubyObj;
    }

    private IRubyObject readObjectDirectly(Value value, MessagePackState state, boolean callProc) throws IOException {
        IRubyObject rubyObj = null;
        // FIXME case 'I' 
        if (value.isNil()) {
            value.asNilValue();
            rubyObj = runtime.getNil();
        } else if (value.isBoolean()) {
            boolean bool = ((BooleanValue) value.asBooleanValue()).getBoolean();
            rubyObj = bool ? runtime.getTrue() : runtime.getFalse();
        } else if (value.isRaw()){
            byte[] bytes = ((RawValue) value.asRawValue()).getByteArray();
            rubyObj = RubyString.newString(runtime, bytes);
        } else if (value.isInteger()) {
            IntegerValue iv = (IntegerValue) value.asIntegerValue();
            try {
        	long l = iv.getLong();
        	rubyObj = RubyFixnum.newFixnum(runtime, l);
            } catch (MessageTypeException e) {
        	BigInteger bi = iv.getBigInteger();
        	rubyObj = RubyBignum.newBignum(runtime, bi);
            }
        } else if (value.isFloat()) {
            double d = ((FloatValue) value.asFloatValue()).getDouble();
            rubyObj = RubyFloat.newFloat(runtime, d);
        } 
        // FIXME RubyRegexp
        // FIXME RubySymbol
        else if (value.isArray()) {
            ArrayValue arrayValue = value.asArrayValue();
            Value[] values = arrayValue.getElementArray();
            int length = values.length;
            RubyArray array = getRuntime().newArray(length);
            registerLinkTarget(array);
            for (int i = 0; i < length; ++i) {
        	IRubyObject element = this.readObjectDirectly(values[i], state, callProc);
        	array.append(element);
            }
            rubyObj = (IRubyObject) array;
        } else if (value.isMap()) {
            MapValue mapValue = value.asMapValue();
            RubyHash hash = RubyHash.newHash(getRuntime());
            registerLinkTarget(hash);
            for (Map.Entry<Value, Value> e : mapValue.entrySet()) {
        	// TODO
        	hash.fastASetCheckString(getRuntime(),
        		readObjectDirectly(e.getKey(), state, callProc),
        		readObjectDirectly(e.getValue(), state, callProc));
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
            int read = inputStream.read(buffer, readLength, length - readLength);

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

    public void defaultVariablesUnmarshal(IRubyObject object) throws IOException {
        int count = unmarshalInt();

        RubyClass cls = object.getMetaClass().getRealClass();
        for (int i = count; --i >= 0; ) {
            IRubyObject key = readObject(false);
            if (i == 0) { // first ivar
                if (runtime.is1_9()
                        && (object instanceof RubyString || object instanceof RubyRegexp)
                        && count >= 1) { // 1.9 string encoding
                    EncodingCapable strObj = (EncodingCapable)object;

                    if (key.asJavaString().equals(MarshalStream.SYMBOL_ENCODING_SPECIAL)) {
                        // special case for USASCII and UTF8
                        if (readObject().isTrue()) {
                            strObj.setEncoding(UTF8Encoding.INSTANCE);
                        } else {
                            strObj.setEncoding(USASCIIEncoding.INSTANCE);
                        }
                        continue;
                    } else if (key.asJavaString().equals("encoding")) {
                        // read off " byte for the string
                        read();
                        ByteList encodingName = unmarshalString();
                        Entry entry = runtime.getEncodingService().findEncodingOrAliasEntry(encodingName);
                        if (entry == null) {
                            throw runtime.newArgumentError("invalid encoding in marshaling stream: " + encodingName);
                        }
                        Encoding encoding = entry.getEncoding();
                        strObj.setEncoding(encoding);
                        continue;
                    } // else fall through as normal ivar
                }
            }
            String name = key.asJavaString();
            IRubyObject value = readObject();

            cls.getVariableAccessorForWrite(name).set(object, value);
        }
    }

    public int read() throws IOException {
        return inputStream.read();
    }
}
