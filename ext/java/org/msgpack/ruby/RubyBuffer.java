//
// MessagePack for Ruby
//
// Copyright (C) 2008-2012 FURUHASHI Sadayuki
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
package org.msgpack.ruby;

import java.io.EOFException;
import java.io.IOException;

import org.jruby.Ruby;
import org.jruby.RubyClass;
import org.jruby.RubyObject;
import org.jruby.RubyString;
import org.jruby.RubyIO;
import org.jruby.runtime.builtin.IRubyObject;
import org.jruby.runtime.ObjectAllocator;
import org.jruby.runtime.ThreadContext;
import org.jruby.runtime.Visibility;
import org.jruby.anno.JRubyClass;
import org.jruby.anno.JRubyMethod;

@JRubyClass(name = "Buffer")
public class RubyBuffer extends RubyObject {
    static final public ObjectAllocator ALLOCATOR = new ObjectAllocator() {
        public IRubyObject allocate(Ruby runtime, RubyClass metaClass) {
            return new RubyBuffer(runtime, metaClass);
        }
    };

    private RubyBufferImpl impl;

    public RubyBuffer(Ruby runtime, RubyClass metaClass) {
        super(runtime, metaClass);
    }

    @JRubyMethod(name = "initialize", visibility = Visibility.PRIVATE)
    public IRubyObject initialize(ThreadContext context) {
    }

    @JRubyMethod(name = "clear")
    public IRubyObject clear(ThreadContext context) {
        return null;  // TODO
    }

    @JRubyMethod(name = "size")
    public IRubyObject size(ThreadContext context) {
        return null;  // TODO
    }

    @JRubyMethod(name = "empty?")
    public IRubyObject emptyP(ThreadContext context) {
        return null;  // TODO
    }

    @JRubyMethod(name = "write", required = 1)
    public IRubyObject write(ThreadContext context, IRubyObject data) {
        return null;  // TODO
    }

    @JRubyMethod(name = "<<", required = 1)
    public IRubyObject append(ThreadContext context, IRubyObject data) {
        return null;  // TODO
    }

    @JRubyMethod(name = "read", required = 1, optional = 1)
    public IRubyObject read(ThreadContext context, IRubyObject n, IRubyObject buffer) {
        return null;  // TODO
    }

    @JRubyMethod(name = "read_all", required = 1, optional = 1)
    public IRubyObject read_all(ThreadContext context, IRubyObject n, IRubyObject buffer) {
        return null;  // TODO
    }

    @JRubyMethod(name = "skip", required = 1)
    public IRubyObject skip(ThreadContext context, IRubyObject n) {
        return null;  // TODO
    }

    @JRubyMethod(name = "skip_all", required = 1)
    public IRubyObject skip_all(ThreadContext context, IRubyObject n) {
        return null;  // TODO
    }

    @JRubyMethod(name = "write_to", required = 1)
    public IRubyObject write_to(ThreadContext context, IRubyObject io) {
        return null;  // TODO
    }

    @JRubyMethod(name = "to_str")
    public IRubyObject to_str(ThreadContext context) {
        return null;  // TODO
    }

    @JRubyMethod(name = "to_a")
    public IRubyObject to_a(ThreadContext context) {
        return null;  // TODO
    }
}

