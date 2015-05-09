package org.msgpack.jruby;


import org.jruby.Ruby;
import org.jruby.RubyModule;
import org.jruby.RubyClass;
import org.jruby.RubyString;
import org.jruby.RubyNil;
import org.jruby.RubyBoolean;
import org.jruby.RubyHash;
import org.jruby.runtime.load.Library;
import org.jruby.runtime.builtin.IRubyObject;
import org.jruby.runtime.ThreadContext;
import org.jruby.runtime.Block;
import org.jruby.runtime.Visibility;
import org.jruby.anno.JRubyModule;
import org.jruby.anno.JRubyMethod;
import org.jruby.internal.runtime.methods.CallConfiguration;
import org.jruby.internal.runtime.methods.DynamicMethod;


public class MessagePackLibrary implements Library {
  public void load(Ruby runtime, boolean wrap) {
    RubyModule msgpackModule = runtime.defineModule("MessagePack");
    msgpackModule.defineAnnotatedMethods(MessagePackModule.class);
    RubyClass standardErrorClass = runtime.getStandardError();
    RubyClass unpackErrorClass = msgpackModule.defineClassUnder("UnpackError", standardErrorClass, standardErrorClass.getAllocator());
    RubyClass underflowErrorClass = msgpackModule.defineClassUnder("UnderflowError", unpackErrorClass, unpackErrorClass.getAllocator());
    RubyClass extensionValueClass = msgpackModule.defineClassUnder("ExtensionValue", runtime.getObject(), new ExtensionValue.ExtensionValueAllocator());
    extensionValueClass.defineAnnotatedMethods(ExtensionValue.class);
    RubyClass packerClass = msgpackModule.defineClassUnder("Packer", runtime.getObject(), new Packer.PackerAllocator());
    packerClass.defineAnnotatedMethods(Packer.class);
    RubyClass unpackerClass = msgpackModule.defineClassUnder("Unpacker", runtime.getObject(), new Unpacker.UnpackerAllocator());
    unpackerClass.defineAnnotatedMethods(Unpacker.class);
    RubyClass bufferClass = msgpackModule.defineClassUnder("Buffer", runtime.getObject(), new Buffer.BufferAllocator());
    bufferClass.defineAnnotatedMethods(Buffer.class);
    installCoreExtensions(runtime);
  }

  private void installCoreExtensions(Ruby runtime) {
    installCoreExtensions(
      runtime,
      runtime.getNilClass(),
      runtime.getTrueClass(),
      runtime.getFalseClass(),
      runtime.getFixnum(),
      runtime.getBignum(),
      runtime.getFloat(),
      runtime.getString(),
      runtime.getArray(),
      runtime.getHash(),
      runtime.getSymbol()
    );
  }

  private void installCoreExtensions(Ruby runtime, RubyClass... classes) {
    for (RubyClass cls : classes) {
      cls.addMethod("to_msgpack", createToMsgpackMethod(runtime, cls));
    }
  }

  private DynamicMethod createToMsgpackMethod(final Ruby runtime, RubyClass cls) {
    return new DynamicMethod(cls, Visibility.PUBLIC, CallConfiguration.FrameNoneScopeNone) {
      @Override
      public IRubyObject call(ThreadContext context, IRubyObject recv, RubyModule clazz, String name, IRubyObject[] args, Block block) {
        IRubyObject[] allArgs = new IRubyObject[1 + args.length];
        allArgs[0] = recv;
        System.arraycopy(args, 0, allArgs, 1, args.length);
        return MessagePackModule.pack(runtime.getCurrentContext(), null, allArgs);
      }

      @Override
      public DynamicMethod dup() {
        return this;
      }
    };
  }

  @JRubyModule(name = "MessagePack")
  public static class MessagePackModule {
    @JRubyMethod(module = true, required = 1, optional = 1, alias = {"dump"})
    public static IRubyObject pack(ThreadContext ctx, IRubyObject recv, IRubyObject[] args) {
      IRubyObject[] extraArgs = null;
      if (args.length == 0) {
        extraArgs = new IRubyObject[] {};
      } else {
        extraArgs = new IRubyObject[args.length - 1];
        System.arraycopy(args, 1, extraArgs, 0, args.length - 1);
      }
      Packer packer = new Packer(ctx.getRuntime(), ctx.getRuntime().getModule("MessagePack").getClass("Packer"));
      packer.initialize(ctx, extraArgs);
      packer.write(ctx, args[0]);
      return packer.toS(ctx);
    }

    @JRubyMethod(module = true, required = 1, optional = 1, alias = {"load"})
    public static IRubyObject unpack(ThreadContext ctx, IRubyObject recv, IRubyObject[] args) {
      Decoder decoder = new Decoder(ctx.getRuntime(), args[0].asString().getBytes());
      if (args.length > 1 && !args[args.length - 1].isNil()) {
        RubyHash hash = args[args.length - 1].convertToHash();
        IRubyObject symbolizeKeys = hash.fastARef(ctx.getRuntime().newSymbol("symbolize_keys"));
        decoder.symbolizeKeys(symbolizeKeys != null && symbolizeKeys.isTrue());
      }
      return decoder.next();
    }
  }
}
