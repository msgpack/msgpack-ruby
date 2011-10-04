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
import org.msgpack.packer.BufferPacker;
import org.msgpack.template.Templates;


public final class RubyObjectPacker {

    private final Ruby runtime;

    private int depth = 0;

    public RubyObjectPacker(Ruby runtime) throws IOException {
        this.runtime = runtime;
    }

    public void writeRubyObject(BufferPacker packer, IRubyObject value) throws IOException {
        depth++;
        writeRubyObjectDirectly(packer, value);
        depth--;
        if (depth == 0) {
            packer.flush();
        }
    }

    public void writeRubyObjectDirectly(BufferPacker packer, IRubyObject value) throws IOException {
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

    private void writeNil(BufferPacker packer) throws IOException {
	packer.writeNil();
    }

    private void writeString(BufferPacker packer, RubyString string) throws IOException {
	Templates.TString.write(packer, string.asJavaString());
    }

    private void writeTrue(BufferPacker packer) throws IOException {
	Templates.TBoolean.write(packer, true);
    }

    private void writeFalse(BufferPacker packer) throws IOException {
	Templates.TBoolean.write(packer, false);
    }

    private void writeFixnum(BufferPacker packer, RubyFixnum fixnum) throws IOException {
	Templates.TLong.write(packer, fixnum.getLongValue());
    }

    private void writeFloat(BufferPacker packer, RubyFloat f) throws IOException {
	Templates.TDouble.write(packer, f.getValue());
    }

    private void writeBignum(BufferPacker packer, RubyBignum bignum) throws IOException {
	Templates.TBigInteger.write(packer, bignum.getValue());
    }

    private void writeArray(BufferPacker packer, RubyArray array) throws IOException {
        int length = array.getLength();
        packer.writeArrayBegin(length);
        for (int i = 0; i < length; ++i) {
            writeRubyObject(packer, array.eltInternal(i));
        }
        packer.writeArrayEnd();
    }

    @SuppressWarnings({ "rawtypes" })
    private void writeHash(BufferPacker packer, RubyHash hash) throws IOException {
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

    public byte[] toByteArray(BufferPacker packer) {
	return packer.toByteArray();
    }
}