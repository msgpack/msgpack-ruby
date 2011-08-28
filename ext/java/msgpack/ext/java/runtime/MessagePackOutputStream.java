package msgpack.ext.java.runtime;

import java.io.FilterOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

import org.jcodings.Encoding;
import org.jcodings.specific.ASCIIEncoding;
import org.jcodings.specific.USASCIIEncoding;
import org.jcodings.specific.UTF8Encoding;
import org.jruby.IncludedModuleWrapper;
import org.jruby.Ruby;
import org.jruby.RubyArray;
import org.jruby.RubyBignum;
import org.jruby.RubyBoolean;
import org.jruby.RubyClass;
import org.jruby.RubyFixnum;
import org.jruby.RubyFloat;
import org.jruby.RubyHash;
import org.jruby.RubyModule;
import org.jruby.RubyRegexp;
import org.jruby.RubyString;
import org.jruby.internal.runtime.methods.DynamicMethod;
import org.jruby.runtime.ClassIndex;
import org.jruby.runtime.builtin.IRubyObject;
import org.jruby.runtime.builtin.Variable;
import org.jruby.runtime.encoding.EncodingCapable;
import org.jruby.runtime.marshal.CoreObjectType;
import org.jruby.runtime.marshal.DataType;
import org.jruby.util.ByteList;
import org.msgpack.MessagePack;
import org.msgpack.packer.Packer;


public final class MessagePackOutputStream extends FilterOutputStream {
    private final Ruby runtime;
    private Packer packer;
    private final MessagePackOutputCache cache;
    private boolean tainted = false;
    private boolean untrusted = false;
    
    private int depth = 0;

    private final static char TYPE_IVAR = 'I';
    private final static char TYPE_USRMARSHAL = 'U';
    private final static char TYPE_USERDEF = 'u';
    private final static char TYPE_UCLASS = 'C';
    public final static String SYMBOL_ENCODING_SPECIAL = "E";
    private final static String SYMBOL_ENCODING = "encoding";

    public MessagePackOutputStream(Ruby runtime, MessagePack msgpack, OutputStream out) throws IOException {
        super(out);
        this.runtime = runtime;
        this.packer = msgpack.createPacker(out);
        this.cache = new MessagePackOutputCache();
    }

    public void writeObject(IRubyObject value) throws IOException {
        depth++;

        tainted |= value.isTaint();
        untrusted |= value.isUntrusted();

        writeAndRegister(value);

        depth--;
        if (depth == 0) {
            packer.close();
            out.flush(); // flush afer whole dump is complete
        }
    }

    private void writeAndRegister(IRubyObject value) throws IOException {
        if (cache.isRegistered(value)) {
            cache.writeLink(this, value);
        } else {
            writeDirectly(value);
        }
    }

    public void registerLinkTarget(IRubyObject newObject) {
        if (shouldBeRegistered(newObject)) {
            cache.register(newObject);
        }
    }

    public void registerSymbol(String sym) {
        cache.registerSymbol(sym);
    }

    static boolean shouldBeRegistered(IRubyObject value) {
        if (value.isNil()) {
            return false;
        } else if (value instanceof RubyBoolean) {
            return false;
        } else if (value instanceof RubyFixnum) {
            return !isMarshalFixnum((RubyFixnum)value);
        }
        return true;
    }

    private static boolean isMarshalFixnum(RubyFixnum fixnum) {
        return fixnum.getLongValue() <= RubyFixnum.MAX_MARSHAL_FIXNUM && fixnum.getLongValue() >= RubyFixnum.MIN_MARSHAL_FIXNUM;
    }

    private void writeAndRegisterSymbol(String sym) throws IOException {
        if (cache.isSymbolRegistered(sym)) {
            cache.writeSymbolLink(this, sym);
        } else {
            registerSymbol(sym);
            dumpSymbol(sym);
        }
    }

    private List<Variable<Object>> getVariables(IRubyObject value) throws IOException {
        List<Variable<Object>> variables = null;
        if (value instanceof CoreObjectType) {
            int nativeTypeIndex = ((CoreObjectType)value).getNativeTypeIndex();
            
            if (nativeTypeIndex != ClassIndex.OBJECT && nativeTypeIndex != ClassIndex.BASICOBJECT) {
                if (shouldMarshalEncoding(value) || (
                        !value.isImmediate()
                        && value.hasVariables()
                        && nativeTypeIndex != ClassIndex.CLASS
                        && nativeTypeIndex != ClassIndex.MODULE
                        )) {
                    // object has instance vars and isn't a class, get a snapshot to be marshalled
                    // and output the ivar header here

                    variables = value.getVariableList();

                    // write `I' instance var signet if class is NOT a direct subclass of Object
                    write(TYPE_IVAR);
                }
                RubyClass type = value.getMetaClass();
                switch(nativeTypeIndex) {
                case ClassIndex.STRING:
                case ClassIndex.REGEXP:
                case ClassIndex.ARRAY:
                case ClassIndex.HASH:
                    type = dumpExtended(type);
                    break;
                }

                if (nativeTypeIndex != value.getMetaClass().index && nativeTypeIndex != ClassIndex.STRUCT) {
                    // object is a custom class that extended one of the native types other than Object
                    writeUserClass(value, type);
                }
            }
        }
        return variables;
    }

    private boolean shouldMarshalEncoding(IRubyObject value) {
        return runtime.is1_9()
                && (value instanceof RubyString || value instanceof RubyRegexp)
                && ((EncodingCapable)value).getEncoding() != ASCIIEncoding.INSTANCE;
    }

    public void writeDirectly(IRubyObject value) throws IOException {
        List<Variable<Object>> variables = getVariables(value);
        writeObjectData(value);
        if (variables != null) {
            if (runtime.is1_9()) {
                dumpVariablesWithEncoding(variables, value);
            } else {
                dumpVariables(variables);
            }
        }
    }

    public static String getPathFromClass(RubyModule clazz) {
        String path = clazz.getName();
        
        if (path.charAt(0) == '#') {
            String classOrModule = clazz.isClass() ? "class" : "module";
            throw clazz.getRuntime().newTypeError("can't dump anonymous " + classOrModule + " " + path);
        }
        
        RubyModule real = clazz.isModule() ? clazz : ((RubyClass)clazz).getRealClass();

        if (clazz.getRuntime().getClassFromPath(path) != real) {
            throw clazz.getRuntime().newTypeError(path + " can't be referred");
        }
        return path;
    }

    private void writeObjectData(IRubyObject value) throws IOException {
        // switch on the object's *native type*. This allows use-defined
        // classes that have extended core native types to piggyback on their
        // marshalling logic.
        if (value instanceof CoreObjectType) {
            if (value instanceof DataType) {
                throw value.getRuntime().newTypeError("no marshal_dump is defined for class " + value.getMetaClass().getName());
            }
            int nativeTypeIndex = ((CoreObjectType)value).getNativeTypeIndex();

            switch (nativeTypeIndex) {
            case ClassIndex.ARRAY:
                writeArray((RubyArray) value);
                return;
            case ClassIndex.FALSE:
        	writeFalse();
                return;
            case ClassIndex.FIXNUM:
        	writeFixnum((RubyFixnum) value);
        	return;
            case ClassIndex.BIGNUM:
        	writeBignum((RubyBignum) value);
                return;
            case ClassIndex.CLASS:
        	// FIXME #MN
        	throw runtime.newTypeError("unsupported error #MN");
//                if (((RubyClass)value).isSingleton()) throw runtime.newTypeError("singleton class can't be dumped");
//                write('c');
//                RubyClass.marshalTo((RubyClass)value, this);
//                return;
            case ClassIndex.FLOAT:
        	writeFloat((RubyFloat) value);
                return;
            case ClassIndex.HASH: {
                writeHash((RubyHash) value);
                return;
            }
            case ClassIndex.MODULE:
        	// FIXME #MN
        	throw runtime.newTypeError("unsupported error #MN");
//                write('m');
//                RubyModule.marshalTo((RubyModule)value, this);
//                return;
            case ClassIndex.NIL:
        	writeNil();
                return;
            case ClassIndex.OBJECT:
            case ClassIndex.BASICOBJECT:
        	// FIXME #MN
        	throw runtime.newTypeError("unsupported error #MN");
//                dumpDefaultObjectHeader(value.getMetaClass());
//                value.getMetaClass().getRealClass().marshal(value, this);
//                return;
            case ClassIndex.REGEXP:
        	// FIXME #MN
        	throw runtime.newTypeError("unsupported error #MN");
//                write('/');
//                RubyRegexp.marshalTo((RubyRegexp)value, this);
//                return;
            case ClassIndex.STRING:
        	writeString((RubyString) value);
                return;
            case ClassIndex.STRUCT:
        	// FIXME #MN
        	throw runtime.newTypeError("unsupported error #MN");
//                RubyStruct.marshalTo((RubyStruct)value, this);
//                return;
            case ClassIndex.SYMBOL:
        	// FIXME #MN
        	throw runtime.newTypeError("unsupported error #MN");
//                writeAndRegisterSymbol(((RubySymbol)value).asJavaString());
//                return;
            case ClassIndex.TRUE:
        	writeTrue();
                return;
            default:
        	throw runtime.newTypeError("can't dump " + value.getMetaClass().getName());
            }
        } else {
            // FIXME
//            dumpDefaultObjectHeader(value.getMetaClass());
//            value.getMetaClass().getRealClass().marshal(value, this);
        }
    }

    private void writeArray(RubyArray array) throws IOException {
	registerLinkTarget(array);

        int length = array.getLength();
        try {
            packer.writeArrayBegin(length);
            for (int i = 0; i < length; ++i) {
        	writeObject(array.eltInternal(i));
            }
            packer.writeArrayEnd();
        } catch (ArrayIndexOutOfBoundsException e) {
            throw runtime.newConcurrencyError(
        	    "Detected invalid array contents due to unsynchronized modifications with concurrent users");
        }
    }

    private void writeFalse() throws IOException {
	packer.writeBoolean(false);
    }

    private void writeFixnum(RubyFixnum fixnum) throws IOException {
	packer.writeLong(fixnum.getLongValue());
    }

    private void writeBignum(RubyBignum bignum) throws IOException {
	packer.writeBigInteger(bignum.getValue());
    }

    private void writeFloat(RubyFloat f) throws IOException {
	packer.writeDouble(f.getValue());
    }

    private void writeHash(RubyHash hash) throws IOException {
	registerLinkTarget(hash);
	int size = hash.size();
	packer.writeMapBegin(size);
	Iterator iter = hash.directEntrySet().iterator();
	while (iter.hasNext()) {
	    Map.Entry e = (Map.Entry) iter.next();
	    IRubyObject key = (IRubyObject) e.getKey();
	    writeObject(key);
	    IRubyObject val = (IRubyObject) e.getValue();
	    writeObject(val);
	    
	}
	packer.writeMapEnd();
    }

    private void writeNil() throws IOException {
	packer.writeNil();
    }

    private void writeString(RubyString string) throws IOException {
	registerLinkTarget(string);
	packer.writeByteArray(string.getBytes());
    }

    private void writeTrue() throws IOException {
	packer.writeBoolean(true);
    }

    public void writeUserClass(IRubyObject obj, RubyClass type) throws IOException {
        write(TYPE_UCLASS);
        
        // w_unique
        if (type.getName().charAt(0) == '#') {
            throw obj.getRuntime().newTypeError("can't dump anonymous class " + type.getName());
        }
        
        // w_symbol
        writeAndRegisterSymbol(type.getName());
    }
    
    public void dumpVariablesWithEncoding(List<Variable<Object>> vars, IRubyObject obj) throws IOException {
        if (shouldMarshalEncoding(obj)) {
            writeInt(vars.size() + 1); // vars preceded by encoding
            writeEncoding(((EncodingCapable)obj).getEncoding());
        } else {
            writeInt(vars.size());
        }
        
        dumpVariablesShared(vars);
    }

    public void dumpVariables(List<Variable<Object>> vars) throws IOException {
        writeInt(vars.size());
        dumpVariablesShared(vars);
    }

    private void dumpVariablesShared(List<Variable<Object>> vars) throws IOException {
        for (Variable<Object> var : vars) {
            if (var.getValue() instanceof IRubyObject) {
                writeAndRegisterSymbol(var.getName());
                writeObject((IRubyObject)var.getValue());
            }
        }
    }

    public void writeEncoding(Encoding encoding) throws IOException {
        if (encoding == null || encoding == USASCIIEncoding.INSTANCE) {
            writeAndRegisterSymbol(SYMBOL_ENCODING_SPECIAL);
            writeObjectData(runtime.getFalse());
        } else if (encoding == UTF8Encoding.INSTANCE) {
            writeAndRegisterSymbol(SYMBOL_ENCODING_SPECIAL);
            writeObjectData(runtime.getTrue());
        } else {
            writeAndRegisterSymbol(SYMBOL_ENCODING);
            byte[] name = encoding.getName();
            write('"');
            writeString(new ByteList(name, false));
        }
    }
    
    private boolean hasSingletonMethods(RubyClass type) {
        for(DynamicMethod method : type.getMethods().values()) {
            // We do not want to capture cached methods
            if(method.getImplementationClass() == type) {
                return true;
            }
        }
        return false;
    }

    /** w_extended
     * 
     */
    private RubyClass dumpExtended(RubyClass type) throws IOException {
        if(type.isSingleton()) {
            if (hasSingletonMethods(type) || type.hasVariables()) { // any ivars, since we don't have __attached__ ivar now
                throw type.getRuntime().newTypeError("singleton can't be dumped");
            }
            type = type.getSuperClass();
        }
        while(type.isIncluded()) {
            write('e');
            writeAndRegisterSymbol(((IncludedModuleWrapper)type).getNonIncludedClass().getName());
            type = type.getSuperClass();
        }
        return type;
    }

    public void dumpDefaultObjectHeader(RubyClass type) throws IOException {
        dumpDefaultObjectHeader('o',type);
    }

    public void dumpDefaultObjectHeader(char tp, RubyClass type) throws IOException {
        dumpExtended(type);
        write(tp);
        writeAndRegisterSymbol(getPathFromClass(type.getRealClass()));
    }

    public void writeString(String value) throws IOException {
        writeInt(value.length());
        // FIXME: should preserve unicode?
        out.write(RubyString.stringToBytes(value));
    }

    public void writeString(ByteList value) throws IOException {
        int len = value.length();
        writeInt(len);
        out.write(value.getUnsafeBytes(), value.begin(), len);
    }

    public void dumpSymbol(String value) throws IOException {
        write(':');
        writeString(value);
    }

    public void writeInt(int value) throws IOException {
        if (value == 0) {
            out.write(0);
        } else if (0 < value && value < 123) {
            out.write(value + 5);
        } else if (-124 < value && value < 0) {
            out.write((value - 5) & 0xff);
        } else {
            byte[] buf = new byte[4];
            int i = 0;
            for (; i < buf.length; i++) {
                buf[i] = (byte)(value & 0xff);
                
                value = value >> 8;
                if (value == 0 || value == -1) {
                    break;
                }
            }
            int len = i + 1;
            out.write(value < 0 ? -len : len);
            out.write(buf, 0, i + 1);
        }
    }

    public void writeByte(int value) throws IOException {
        out.write(value);
    }

    public boolean isTainted() {
        return tainted;
    }

    public boolean isUntrusted() {
        return untrusted;
    }
}