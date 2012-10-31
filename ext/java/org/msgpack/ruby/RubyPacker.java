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

@JRubyClass(name = "Packer")
public class RubyPacker extends RubyObject {
    @JRubyMethod(name = "initialize", optional = 2, visibility = Visibility.PRIVATE)
    public IRubyObject initialize(ThreadContext context, IRubyObject arg1, IRubyObject arg2) {
    }

    @JRubyMethod(name = "buffer")
    public IRubyObject buffer(ThreadContext context) {
        return null;  // TODO
    }

    @JRubyMethod(name = "write", required = 1)
    public IRubyObject write(ThreadContext context, IRubyObject obj) {
        return null;  // TODO
    }

    @JRubyMethod(name = "pack", required = 1)
    public IRubyObject pack(ThreadContext context, IRubyObject obj) {
        return write(obj);
    }

    @JRubyMethod(name = "write_nil")
    public IRubyObject write_nil(ThreadContext context) {
        return null;  // TODO
    }

    @JRubyMethod(name = "write_array_header", required = 1)
    public IRubyObject write_array_header(ThreadContext context, IRubyObject size) {
        return null;  // TODO
    }

    @JRubyMethod(name = "write_map_header", required = 1)
    public IRubyObject write_map_header(ThreadContext context, IRubyObject size) {
        return null;  // TODO
    }

    @JRubyMethod(name = "flush")
    public IRubyObject flush(ThreadContext context) {
        return null;  // TODO
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

    @JRubyMethod(name = "to_str")
    public IRubyObject to_str(ThreadContext context) {
        return null;  // TODO
    }

    @JRubyMethod(name = "to_a")
    public IRubyObject to_a(ThreadContext context) {
        return null;  // TODO
    }

    @JRubyMethod(name = "write_to", required = 1)
    public IRubyObject write_to(ThreadContext context, IRubyObject io) {
        return null;  // TODO
    }
}

