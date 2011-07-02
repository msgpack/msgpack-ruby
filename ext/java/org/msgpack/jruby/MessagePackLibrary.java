package org.msgpack.jruby;


import java.io.IOException;

import org.jruby.Ruby;
import org.jruby.RubyModule;
import org.jruby.RubyClass;
import org.jruby.RubyString;
import org.jruby.RubyHash;
import org.jruby.runtime.load.Library;
import org.jruby.runtime.builtin.IRubyObject;
import org.jruby.runtime.ThreadContext;
import org.jruby.anno.JRubyModule;
import org.jruby.anno.JRubyMethod;


public class MessagePackLibrary implements Library {
  public void load(Ruby runtime, boolean wrap) throws IOException {
    RubyModule msgpackModule = runtime.defineModule("MessagePack");
    msgpackModule.defineAnnotatedMethods(MessagePackModule.class);
  }

  @JRubyModule(name = "MessagePack")
  public static class MessagePackModule {
    @JRubyMethod(module = true, required = 1, optional = 1, alias = {"dump"})
    public static IRubyObject pack(ThreadContext ctx, IRubyObject recv, IRubyObject[] args) throws IOException {
      Encoder encoder = new Encoder(ctx.getRuntime());
      return encoder.encode(args[0]);
    }

    @JRubyMethod(module = true, required = 1, optional = 1, alias = {"load"})
    public static IRubyObject unpack(ThreadContext ctx, IRubyObject recv, IRubyObject[] args) throws IOException {
      Decoder decoder = new Decoder(ctx.getRuntime(), args[0].asString().getBytes());
      return decoder.next();
    }
  }
}
