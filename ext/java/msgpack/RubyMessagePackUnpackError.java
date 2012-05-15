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

import org.jruby.Ruby;
import org.jruby.RubyClass;
import org.jruby.RubyObject;
import org.jruby.anno.JRubyClass;
import org.jruby.exceptions.RaiseException;
import org.jruby.runtime.ObjectAllocator;
import org.jruby.runtime.builtin.IRubyObject;

@JRubyClass(name="UnpackError")
@SuppressWarnings("serial")
public class RubyMessagePackUnpackError extends RubyObject {
    static public RubyClass ERROR;

    static final public ObjectAllocator ALLOCATOR = new ObjectAllocator() {
        public IRubyObject allocate(Ruby runtime, RubyClass clazz) {
            return new RubyMessagePackUnpackError(runtime, clazz);
        }
    };

    public RubyMessagePackUnpackError(Ruby runtime, RubyClass clazz) {
	super(runtime, clazz);
    }

    public RaiseException newUnpackError(Ruby runtime, String message) {
        return new RaiseException(runtime, ERROR, message, true);
    }
}
