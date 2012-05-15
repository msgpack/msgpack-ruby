//
// MessagePack for Ruby
//
// Copyright (C) 2008-2012 Muga Nishizawa
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
import java.io.EOFException;
import java.io.IOException;

import msgpack.template.RubyObjectTemplate;

import org.jruby.Ruby;
import org.jruby.RubyArray;
import org.jruby.RubyClass;
import org.jruby.RubyEnumerator;
import org.jruby.RubyIO;
import org.jruby.RubyNil;
import org.jruby.RubyNumeric;
import org.jruby.RubyObject;
import org.jruby.RubyString;
import org.jruby.RubyStringIO;
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
import org.jruby.util.IOInputStream;
import org.msgpack.MessagePack;
import org.msgpack.type.Value;
import org.msgpack.unpacker.BufferUnpacker;
import org.msgpack.unpacker.MessagePackBufferUnpacker;
import org.msgpack.unpacker.MessagePackUnpacker;

@SuppressWarnings("serial")
@JRubyClass(name = "Unpacker")
public class RubyMessagePackUnpacker extends RubyObject {

    static final public ObjectAllocator ALLOCATOR = new ObjectAllocator() {
        public IRubyObject allocate(Ruby runtime, RubyClass clazz) {
            return new RubyMessagePackUnpacker(runtime, clazz);
        }
    };

    private MessagePackBufferUnpacker bufferUnpacker;

    private MessagePackUnpacker streamUnpacker;

    private IRubyObject stream;

    private IRubyObject data;

    public RubyMessagePackUnpacker(Ruby runtime, RubyClass clazz) {
	super(runtime, clazz);
	bufferUnpacker = null;
	streamUnpacker = null;
	stream = null;
	data = null;
    }

    @JRubyMethod(name = "initialize", optional = 1, visibility = Visibility.PRIVATE)
    public IRubyObject initialize(ThreadContext context, IRubyObject[] args) {
	if (args.length > 0) {
	    setStream(context, args[0]);
	}
	return this;
    }

    @JRubyMethod(name = "stream")
    public IRubyObject getStream(ThreadContext context) {
	if (stream == null) {
	    return context.getRuntime().getNil();
	} else {
	    return stream;
	}
    }

    @JRubyMethod(name = "stream=", required = 1)
    public IRubyObject setStream(ThreadContext context, IRubyObject io) {
	bufferUnpacker = null;
	this.stream = io;
	MessagePack msgpack = RubyMessagePack.getMessagePack(context.getRuntime());
	if (io instanceof RubyStringIO) {
	    // TODO: RubyStringIO returns negative numbers when read through IOInputStream#read
	    IRubyObject str = ((RubyStringIO) io).string();
	    byte[] bytes = ((RubyString) str).getBytes();
	    streamUnpacker = new MessagePackUnpacker(msgpack, new ByteArrayInputStream(bytes));
	} else {
	    streamUnpacker = new MessagePackUnpacker(msgpack, new IOInputStream(io));
	}
	return getStream(context);

    }

    @JRubyMethod(name = "data")
    public IRubyObject getData(ThreadContext context) {
	if (data == null) {
	    return context.getRuntime().getNil();
	} else {
	    return data;
	}
    }

    @JRubyMethod(name = "execute")
    public IRubyObject execute(ThreadContext context, IRubyObject data, IRubyObject offset) {
	return executeLimit(context, data, offset, new RubyNil(context.getRuntime()));
    }

    @JRubyMethod(name = "execute_limit")
    public IRubyObject executeLimit(ThreadContext context, IRubyObject data,
	    IRubyObject offset, IRubyObject limit) {
	this.data = null;
	try {
	    int jOffset = RubyNumeric.fix2int(offset);
	    int jLimit = -1;
	    if (!(limit instanceof RubyNil)) {
		jLimit = RubyNumeric.fix2int(limit);
	    }

	    byte[] bytes = data.asString().getBytes();
	    Ruby runtime = context.getRuntime();
	    MessagePack msgpack = RubyMessagePack.getMessagePack(runtime);
	    MessagePackBufferUnpacker bufferUnpacker0 =
		new MessagePackBufferUnpacker(msgpack, bytes.length);
	    bufferUnpacker0.wrap(bytes, jOffset, jLimit == -1 ? bytes.length - jOffset : jLimit);
	    RubyObjectTemplate tmpl = new RubyObjectTemplate(runtime);
	    this.data = tmpl.read(bufferUnpacker0, null);
	    return context.getRuntime().newFixnum(jOffset + bufferUnpacker0.getReadByteCount());
	} catch (IOException ioe) {
	    ioe.printStackTrace();
	    // TODO: how to throw Ruby exceptions?
	    return context.getRuntime().getNil();
	}
    }

    @JRubyMethod(name = "feed")
    public IRubyObject feed(ThreadContext context, IRubyObject data) {
	streamUnpacker = null;
	byte[] bytes = data.asString().getBytes();
	if (bufferUnpacker == null) {
	    MessagePack msgpack = RubyMessagePack.getMessagePack(context.getRuntime());
	    bufferUnpacker = new MessagePackBufferUnpacker(msgpack);
	}
	bufferUnpacker.feed(bytes);
	return context.getRuntime().getNil();
    }

    @JRubyMethod(name = "feed_each")
    public IRubyObject feedEach(ThreadContext context, IRubyObject data, Block block) {
	feed(context, data);
	each(context, block);
	return context.getRuntime().getNil();
    }

    @JRubyMethod(name = "each")
    public IRubyObject each(ThreadContext context, Block block) {
	MessagePackUnpacker bufferUnpacker0 = null;
	if (bufferUnpacker == null && streamUnpacker != null) {
	    bufferUnpacker0 = streamUnpacker;
	} else if (bufferUnpacker != null) {
	    bufferUnpacker0 = bufferUnpacker;
	} else {
	    return context.getRuntime().getNil();
	}
	if (block.isGiven()) {
	    try {
		for (Value value : bufferUnpacker0) {
		    RubyObjectTemplate tmpl = new RubyObjectTemplate(context.getRuntime());
		    IRubyObject rubyObject = tmpl.read(bufferUnpacker0, null);
		    block.yield(context, rubyObject);
		}
	    } catch (IOException e) {
		e.printStackTrace();
	    }
	    return context.getRuntime().getNil();
	} else {
	    return RubyEnumerator.RubyEnumeratorKernel.obj_to_enum(context, this, Block.NULL_BLOCK);
	}
    }

    @JRubyMethod(name = "fill")
    public IRubyObject fill(ThreadContext context, IRubyObject data) {
	return context.getRuntime().getNil();
    }

    @JRubyMethod(name = "finished?")
    public IRubyObject finished(ThreadContext context) {
	return data == null ? context.getRuntime().getFalse() : context.getRuntime().getTrue();
    }

    @JRubyMethod(name = "reset")
    public IRubyObject reset(ThreadContext context) {
	if (bufferUnpacker != null) {
	    bufferUnpacker.reset();
	}
	if (streamUnpacker != null) {
	    streamUnpacker.reset();
	}
	return context.getRuntime().getNil();
    }
}
