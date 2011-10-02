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
package msgpack;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;

import msgpack.runtime.MessagePackInputStream;

import org.jruby.Ruby;
import org.jruby.RubyArray;
import org.jruby.RubyClass;
import org.jruby.RubyIO;
import org.jruby.RubyNil;
import org.jruby.RubyObject;
import org.jruby.RubyString;
import org.jruby.anno.JRubyClass;
import org.jruby.anno.JRubyMethod;
import org.jruby.exceptions.RaiseException;
import org.jruby.runtime.Block;
import org.jruby.runtime.ClassIndex;
import org.jruby.runtime.ObjectAllocator;
import org.jruby.runtime.ThreadContext;
import org.jruby.runtime.Visibility;
import org.jruby.runtime.builtin.IRubyObject;
import org.jruby.runtime.marshal.CoreObjectType;
import org.jruby.util.ByteList;


@SuppressWarnings("serial")
@JRubyClass(name = "Unpacker")
public class RubyMessagePackUnpacker extends RubyObject {

    static final public ObjectAllocator ALLOCATOR = new ObjectAllocator() {
        public IRubyObject allocate(Ruby runtime, RubyClass clazz) {
            return new RubyMessagePackUnpacker(runtime, clazz);
        }
    };

    private MessagePackInputStream msgpackIn = null;

    public RubyMessagePackUnpacker(Ruby runtime, RubyClass clazz) {
	super(runtime, clazz);
    }

    @JRubyMethod(name = "initialize", optional = 1, visibility = Visibility.PRIVATE)
    public IRubyObject initialize(ThreadContext context, IRubyObject io) {
	return streamEq(context, io);
    }

    @JRubyMethod
    public IRubyObject data(ThreadContext context) {
	throw context.getRuntime().newNotImplementedError("data method"); // FIXME #MN
    }

    @JRubyMethod
    public IRubyObject execute(ThreadContext context, IRubyObject data, IRubyObject offset) {
	throw context.getRuntime().newNotImplementedError("execute method"); // FIXME #MN
    }

    @JRubyMethod
    public IRubyObject execute_limit(ThreadContext context, IRubyObject data, IRubyObject offset, IRubyObject limit) {
	throw context.getRuntime().newNotImplementedError("execute_limit method"); // FIXME #MN
    }

    @JRubyMethod
    public IRubyObject feed(ThreadContext context, IRubyObject data) {
	reset(context);
	Ruby runtime = context.getRuntime();
	IRubyObject v = data.checkStringType();

	if (v.isNil()) {
            throw runtime.newTypeError("string instance needed");
        }

        ByteList bytes = ((RubyString) v).getByteList();
        try {
            InputStream in = new ByteArrayInputStream(bytes.getUnsafeBytes(), bytes.begin(), bytes.length());
	    msgpackIn = new MessagePackInputStream(runtime, in, null, true, true);
	} catch (IOException e) {
	    throw context.getRuntime().newArgumentError("data is illegal");
	}
        return new RubyNil(runtime);
    }

    @JRubyMethod
    public IRubyObject feed_each(ThreadContext context, IRubyObject data, Block block) {
	reset(context);
	feed(context, data);
        return eachCommon(context, block);
    }

    @JRubyMethod
    public IRubyObject each(ThreadContext context, Block block) {
	if (block.isGiven()) {
	    return eachCommon(context, block);
	} else {
	    //return enumeratorize(context.getRuntime(), this, "each");
	    throw context.getRuntime().newNotImplementedError("enumeratorize each"); // FIXME #MN
	}
    }

    private IRubyObject eachCommon(ThreadContext context, Block block) {
	Ruby runtime = context.getRuntime();

	if (!block.isGiven()) {
	    throw runtime.newLocalJumpErrorNoBlock();
	}

        if (msgpackIn == null) {
            throw new RaiseException(runtime, RubyMessagePackUnpackError.ERROR, "invoke feed method in advance", true);
        }

	try {
	    IRubyObject value = msgpackIn.readObject();
	    int nativeTypeIndex = ((CoreObjectType) value).getNativeTypeIndex();
	    if (nativeTypeIndex == ClassIndex.ARRAY) {
		RubyArray arrayValue = (RubyArray) value;
		arrayValue.each(context, block);
	    } else {
		block.yield(context, value);
	    }
	} catch (IOException e) {
	    throw new RaiseException(runtime, RubyMessagePackUnpackError.ERROR, "cannot convert data", true);
	}
        return this;
    }

    @JRubyMethod
    public IRubyObject fill(ThreadContext context, IRubyObject data) {
	throw context.getRuntime().newNotImplementedError("fill method"); // FIXME #MN
    }

    @JRubyMethod(name = "finish?")
    public IRubyObject finish(ThreadContext context) {
	throw context.getRuntime().newNotImplementedError("finish method"); // FIXME #MN
    }

    @JRubyMethod
    public IRubyObject reset(ThreadContext context) {
	if (msgpackIn != null) {
	    try {
		msgpackIn.close();
	    } catch (IOException e) {
		throw new RaiseException(context.getRuntime(),
			RubyMessagePackUnpackError.ERROR, "cannot convert data", true);
	    } finally {
		msgpackIn = null;		
	    }
	}
	return this;
    }

    @JRubyMethod
    public IRubyObject stream(ThreadContext context) {
        return new RubyIO(context.getRuntime(), msgpackIn.getInputStream());
    }

    @JRubyMethod(name = "stream=", required = 1)
    public IRubyObject streamEq(ThreadContext context, IRubyObject io) {
	reset(context);
	if (io != null && io instanceof RubyIO) {
	    try {
		InputStream in = ((RubyIO) io).getInStream();
		msgpackIn = new MessagePackInputStream(context.getRuntime(), in, null, io.isTaint(), io.isUntrusted());
	    } catch (IOException e) {
		throw context.getRuntime().newArgumentError("illegal io");
	    }
	}
        return this;
    }
}
