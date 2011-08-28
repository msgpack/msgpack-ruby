package msgpack;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.EOFException;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import msgpack.runtime.MessagePackInputStream;
import msgpack.runtime.MessagePackOutputStream;

import org.jruby.Ruby;
import org.jruby.RubyString;
import org.jruby.anno.JRubyMethod;
import org.jruby.anno.JRubyModule;
import org.jruby.runtime.ThreadContext;
import org.jruby.runtime.builtin.IRubyObject;
import org.jruby.runtime.marshal.MarshalStream;
import org.jruby.runtime.marshal.UnmarshalStream;
import org.jruby.util.ByteList;
import org.jruby.util.IOInputStream;
import org.jruby.util.IOOutputStream;
import org.msgpack.MessagePack;


@JRubyModule(name = "MessagePack")
public class RubyMessagePack {

    private static MessagePack msgpack = new MessagePack();

    @JRubyMethod(name = "pack", required = 1, optional = 1, module = true)
    public static IRubyObject pack(IRubyObject recv, IRubyObject[] args) {
//	System.out.println("invoke pack()");
//	for (IRubyObject arg : args) {
//	    System.out.println(String.format("arg: %s, arg class: %s", new Object[] { arg, arg.getClass().getName() }));
//	}
        Ruby runtime = recv.getRuntime();
        IRubyObject object = args[0];
        IRubyObject io = null;

        if (args.length == 2 && args[1].respondsTo("write")) {
            io = args[1];
        }

        try {
            if (io != null) {
                writeToStream(runtime, object, outputStream(runtime.getCurrentContext(), io));
                return io;
            }
            
            ByteArrayOutputStream out = new ByteArrayOutputStream();
            boolean[] taintUntrust = writeToStream(runtime, object, out);
            RubyString result = RubyString.newString(runtime, new ByteList(out.toByteArray()));

            if (taintUntrust[0]) {
        	result.setTaint(true);
            }
            if (taintUntrust[1]) {
        	result.setUntrusted(true);
            }

            return result;
        } catch (IOException e) {
            throw runtime.newIOErrorFromException(e);
        }
    }

    private static OutputStream outputStream(ThreadContext context, IRubyObject io) {
        setBinmodeIfPossible(context, io);
        return new IOOutputStream(io);
    }

    private static void setBinmodeIfPossible(ThreadContext context, IRubyObject io) {
        if (io.respondsTo("binmode")) {
            io.callMethod(context, "binmode");
        }
    }

    private static boolean[] writeToStream(Ruby runtime, IRubyObject object, OutputStream out) throws IOException {
	//MarshalStream output = new MarshalStream(runtime, rawOutput, depthLimit); // TODO #MN will delete it
        MessagePackOutputStream msgpackOut = new MessagePackOutputStream(runtime, msgpack, out);
        msgpackOut.writeObject(object);
        return new boolean[] { msgpackOut.isTainted(), msgpackOut.isUntrusted() };
    }

    @JRubyMethod(name = "unpack", required = 1, module = true)
    public static IRubyObject unpack(ThreadContext context, IRubyObject recv, IRubyObject io) {
	Ruby runtime = context.getRuntime();

        try {
            InputStream in;
            boolean tainted;
            boolean untrusted;
            IRubyObject v = io.checkStringType();
            
            if (!v.isNil()) {
                tainted = io.isTaint();
                untrusted = io.isUntrusted();
                ByteList bytes = ((RubyString) v).getByteList();
                in = new ByteArrayInputStream(bytes.getUnsafeBytes(), bytes.begin(), bytes.length());
            } else if (io.respondsTo("getc") && io.respondsTo("read")) {
                tainted = true;
                untrusted = true;
                in = inputStream(context, io);
            } else {
                throw runtime.newTypeError("instance of IO needed");
            }

            //return new UnmarshalStream(runtime, rawInput, proc, tainted, untrusted).unmarshalObject();// TODO #MN will delete it
            return new MessagePackInputStream(runtime, msgpack, in, null, tainted, untrusted).readObject();
        } catch (EOFException e) {
            if (io.respondsTo("to_str")) {
        	throw runtime.newArgumentError("packed data too short");
            }
            throw runtime.newEOFError();
        } catch (IOException e) {
            throw runtime.newIOErrorFromException(e);
        }
    }

    private static InputStream inputStream(ThreadContext context, IRubyObject in) {
        setBinmodeIfPossible(context, in);
        return new IOInputStream(in);
    }

    @JRubyMethod(name = "unpack_limit", required = 2, module = true)
    public static IRubyObject unpackLimit(ThreadContext context, IRubyObject recv, IRubyObject io, IRubyObject limit) {
	// FIXME
	Ruby runtime = context.getRuntime();
        throw runtime.newNotImplementedError("unpack_limit");
    }
}
