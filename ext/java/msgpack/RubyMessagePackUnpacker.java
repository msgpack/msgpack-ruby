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

import org.jruby.Ruby;
import org.jruby.RubyClass;
import org.jruby.RubyObject;
import org.jruby.anno.JRubyClass;
import org.jruby.anno.JRubyMethod;
import org.jruby.runtime.ObjectAllocator;
import org.jruby.runtime.ThreadContext;
import org.jruby.runtime.Visibility;
import org.jruby.runtime.builtin.IRubyObject;


@SuppressWarnings("serial")
@JRubyClass(name = "Unpacker")
public class RubyMessagePackUnpacker extends RubyObject {

    static final public ObjectAllocator ALLOCATOR = new ObjectAllocator() {
        public IRubyObject allocate(Ruby runtime, RubyClass clazz) {
            return new RubyMessagePackUnpacker(runtime, clazz);
        }
    };

    public RubyMessagePackUnpacker(Ruby runtime, RubyClass clazz) {
	super(runtime, clazz);
    }

    @JRubyMethod(name = "initialize", optional = 1, visibility = Visibility.PRIVATE)
    public IRubyObject initialize(ThreadContext context, IRubyObject[] args) {
	// FIXME
        return this;
    }

    @JRubyMethod(name = "data")
    public IRubyObject data(ThreadContext context) {
	// FIXME
	return this;
    }
    
//    @JRubyMethod(name = "each", optional = 1)
//    public IRubyObject each(ThreadContext context, IRubyObject[] args) {
////	System.out.println("invoke each()");
////	for (IRubyObject arg : args) {
////	    System.out.println(String.format("arg: %s, arg class: %s", new Object[] { arg, arg.getClass().getName() }));
////	}
//	// FIXME
//        return this;
//    }

    @JRubyMethod(name = "execute", required = 2)
    public IRubyObject execute(ThreadContext context, IRubyObject data, IRubyObject offset) {
	// FIXME
        return this;
    }

    @JRubyMethod(name = "execute_limit", required = 3)
    public IRubyObject executeLimit(ThreadContext context, IRubyObject data, IRubyObject offset, IRubyObject limit) {
	// FIXME
        return this;
    }

    @JRubyMethod(name = "feed", required = 1)
    public IRubyObject feed(ThreadContext context, IRubyObject data) {
	// FIXME
        return this;
    }

//    @JRubyMethod(name = "feed_each", required = 1)
//    public IRubyObject feedEach(ThreadContext context, IRubyObject[] args) {
//	  // FIXME
//        return this;
//    }

    @JRubyMethod(name = "fill")
    public IRubyObject fill(ThreadContext context) {
	// FIXME
	return this;
    }

    @JRubyMethod(name = "finish?")
    public IRubyObject finish(ThreadContext context) {
	// FIXME
	return this;
    }

    @JRubyMethod(name = "reset")
    public IRubyObject reset(ThreadContext context) {
	// FIXME
	return this;
    }

    @JRubyMethod(name = "stream")
    public IRubyObject stream(ThreadContext context) {
	// FIXME
        return this;
    }

    @JRubyMethod(name = "stream=", required = 1)
    public IRubyObject eqstreamEq(ThreadContext context, IRubyObject stream) {
	// FIXME
        return this;
    }
}
