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
    static RubyClass ERROR;

    static final ObjectAllocator ALLOCATOR = new ObjectAllocator() {
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
