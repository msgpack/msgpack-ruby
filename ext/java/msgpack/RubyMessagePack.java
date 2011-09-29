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
import java.io.ByteArrayOutputStream;
import java.io.EOFException;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.HashMap;
import java.util.Map;

import msgpack.runtime.MessagePackInputStream;
import msgpack.runtime.MessagePackOutputStream;

import org.jruby.Ruby;
import org.jruby.RubyString;
import org.jruby.anno.JRubyMethod;
import org.jruby.anno.JRubyModule;
import org.jruby.runtime.ThreadContext;
import org.jruby.runtime.builtin.IRubyObject;
import org.jruby.util.ByteList;
import org.jruby.util.IOOutputStream;
import org.msgpack.MessagePack;


@JRubyModule(name = "MessagePack")
public class RubyMessagePack {

    private static Map<Ruby, MessagePack> msgpacks = new HashMap<Ruby, MessagePack>();

    public static MessagePack getMessagePack(Ruby runtime) {
	MessagePack msgpack = msgpacks.get(runtime);
	if (msgpack == null) {
	    msgpack = new MessagePack();
	    msgpacks.put(runtime, msgpack);
	}
	return msgpack;
    }

    @JRubyMethod(name = "pack", required = 1, optional = 1, module = true)
    public static IRubyObject pack(IRubyObject recv, IRubyObject[] args) {
        Ruby runtime = recv.getRuntime();
        IRubyObject object = args[0];
        IRubyObject io = null;

        if (args.length == 2 && args[1].respondsTo("write")) {
            io = args[1];
        }

        try {
            if (io != null) {
        	writeToStream(runtime, object, new IOOutputStream(io));
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

    private static boolean[] writeToStream(Ruby runtime, IRubyObject object, OutputStream out) throws IOException {
	//MarshalStream output = new MarshalStream(runtime, rawOutput, depthLimit); // TODO #MN will delete it
	MessagePackOutputStream msgpackOut = new MessagePackOutputStream(runtime, out);
        msgpackOut.writeObject(object);
        return new boolean[] { msgpackOut.isTainted(), msgpackOut.isUntrusted() };
    }

    @JRubyMethod(name = "unpack", required = 1, module = true)
    public static IRubyObject unpack(ThreadContext context, IRubyObject recv, IRubyObject io) {
	Ruby runtime = context.getRuntime();
        IRubyObject v = io.checkStringType();

        try {
            if (v.isNil()) {
        	throw runtime.newTypeError("instance of IO needed");
            }

            boolean tainted = io.isTaint();
            boolean untrusted = io.isUntrusted();
            ByteList bytes = ((RubyString) v).getByteList();
            InputStream in = new ByteArrayInputStream(bytes.getUnsafeBytes(), bytes.begin(), bytes.length());
            // TODO #MN will delete it
            //return new UnmarshalStream(runtime, rawInput, proc, tainted, untrusted).unmarshalObject();
            return new MessagePackInputStream(runtime, in, null, tainted, untrusted).readObject();
        } catch (EOFException e) {
            if (io.respondsTo("to_str")) {
        	throw runtime.newArgumentError("packed data too short");
            }
            throw runtime.newEOFError();
        } catch (IOException e) {
            throw runtime.newIOErrorFromException(e);
        }
    }

    @JRubyMethod(name = "unpack_limit", required = 2, module = true)
    public static IRubyObject unpackLimit(ThreadContext context, IRubyObject recv, IRubyObject io, IRubyObject limit) {
	// FIXME #MN
	Ruby runtime = context.getRuntime();
        throw runtime.newNotImplementedError("unpack_limit");
    }
}
