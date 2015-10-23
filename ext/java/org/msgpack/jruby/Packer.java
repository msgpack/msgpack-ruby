package org.msgpack.jruby;

import org.jruby.Ruby;
import org.jruby.RubyClass;
import org.jruby.RubyObject;
import org.jruby.RubyArray;
import org.jruby.RubyHash;
import org.jruby.RubyIO;
import org.jruby.RubyInteger;
import org.jruby.RubyFixnum;
import org.jruby.runtime.Block;
import org.jruby.runtime.builtin.IRubyObject;
import org.jruby.anno.JRubyClass;
import org.jruby.anno.JRubyMethod;
import org.jruby.runtime.ThreadContext;
import org.jruby.runtime.ObjectAllocator;
import org.jruby.util.ByteList;

import static org.jruby.runtime.Visibility.PRIVATE;

@JRubyClass(name="MessagePack::Packer")
public class Packer extends RubyObject {
    public ExtensionRegistry registry;
    private Buffer buffer;
    private Encoder encoder;

    public Packer(Ruby runtime, RubyClass type, ExtensionRegistry registry) {
        super(runtime, type);
        this.registry = registry;
    }

    static class PackerAllocator implements ObjectAllocator {
        public IRubyObject allocate(Ruby runtime, RubyClass type) {
            return new Packer(runtime, type, null);
        }
    }

    @JRubyMethod(name = "initialize", optional = 2)
    public IRubyObject initialize(ThreadContext ctx, IRubyObject[] args) {
        boolean compatibilityMode = false;
        if (args.length > 0 && args[args.length - 1] instanceof RubyHash) {
            RubyHash options = (RubyHash) args[args.length - 1];
            IRubyObject mode = options.fastARef(ctx.getRuntime().newSymbol("compatibility_mode"));
            compatibilityMode = (mode != null) && mode.isTrue();
        }
        this.encoder = new Encoder(ctx.getRuntime(), compatibilityMode, registry);
        this.buffer = new Buffer(ctx.getRuntime(), ctx.getRuntime().getModule("MessagePack").getClass("Buffer"));
        this.buffer.initialize(ctx, args);
        this.registry = new ExtensionRegistry();
        return this;
    }

    public static Packer newPacker(ThreadContext ctx, ExtensionRegistry extRegistry, IRubyObject[] args) {
        Packer packer = new Packer(ctx.getRuntime(), ctx.getRuntime().getModule("MessagePack").getClass("Packer"), extRegistry);
        packer.initialize(ctx, args);
        return packer;
    }

    @JRubyMethod(name = "compatibility_mode?")
    public IRubyObject isCompatibilityMode(ThreadContext ctx) {
        return encoder.isCompatibilityMode() ? ctx.getRuntime().getTrue() : ctx.getRuntime().getFalse();
    }

    @JRubyMethod(name = "registered_types_internal", visibility = PRIVATE)
    public IRubyObject registeredTypesInternal(ThreadContext ctx) {
        return registry.toInternalPackerRegistry(ctx);
    }

    @JRubyMethod(name = "register_type", required = 2, optional = 1)
    public IRubyObject registerType(ThreadContext ctx, IRubyObject[] args, final Block block) {
        Ruby runtime = ctx.getRuntime();
        IRubyObject type = args[0];
        IRubyObject klass = args[1];

        IRubyObject arg;
        IRubyObject proc;
        if (args.length == 2) {
            if (! block.isGiven()) {
                throw runtime.newLocalJumpErrorNoBlock();
            }
            proc = block.getProcObject();
            arg = proc;
        } else if (args.length == 3) {
            arg = args[2];
            proc = arg.callMethod(ctx, "to_proc");
        } else {
            throw runtime.newArgumentError(String.format("wrong number of arguments (%d for 2..3)", 2 + args.length));
        }

        long typeId = ((RubyFixnum) type).getLongValue();
        if (typeId < -128 || typeId > 127) {
            throw runtime.newRangeError(String.format("integer %d too big to convert to `signed char'", typeId));
        }

        if (!(klass instanceof RubyClass)) {
            throw runtime.newArgumentError(String.format("expected Class but found %s.", klass.getType().getName()));
        }
        RubyClass extClass = (RubyClass) klass;

        registry.put(extClass, (int) typeId, proc, arg, null, null);
        return runtime.getNil();
    }

    @JRubyMethod(name = "write", alias = { "pack" })
    public IRubyObject write(ThreadContext ctx, IRubyObject obj) {
        buffer.write(ctx, encoder.encode(obj, this));
        return this;
    }

    @JRubyMethod(name = "write_nil")
    public IRubyObject writeNil(ThreadContext ctx) {
        write(ctx, null);
        return this;
    }

    @JRubyMethod(name = "write_array_header")
    public IRubyObject writeArrayHeader(ThreadContext ctx, IRubyObject size) {
        int s = (int) size.convertToInteger().getLongValue();
        buffer.write(ctx, encoder.encodeArrayHeader(s));
        return this;
    }

    @JRubyMethod(name = "write_map_header")
    public IRubyObject writeMapHeader(ThreadContext ctx, IRubyObject size) {
        int s = (int) size.convertToInteger().getLongValue();
        buffer.write(ctx, encoder.encodeMapHeader(s));
        return this;
    }

    @JRubyMethod(name = "to_s", alias = { "to_str" })
    public IRubyObject toS(ThreadContext ctx) {
        return buffer.toS(ctx);
    }

    @JRubyMethod(name = "buffer")
    public IRubyObject buffer(ThreadContext ctx) {
        return buffer;
    }

    @JRubyMethod(name = "flush")
    public IRubyObject flush(ThreadContext ctx) {
        return buffer.flush(ctx);
    }

    @JRubyMethod(name = "size")
    public IRubyObject size(ThreadContext ctx) {
        return buffer.size(ctx);
    }

    @JRubyMethod(name = "clear")
    public IRubyObject clear(ThreadContext ctx) {
        return buffer.clear(ctx);
    }
}
