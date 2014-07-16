package org.msgpack.jruby;


import java.io.InputStream;
import java.io.ByteArrayInputStream;
import java.io.IOException;

import org.jruby.Ruby;
import org.jruby.RubyModule;
import org.jruby.RubyClass;
import org.jruby.RubyString;
import org.jruby.RubyObject;
import org.jruby.RubyHash;
import org.jruby.RubyIO;
import org.jruby.RubyStringIO;
import org.jruby.RubyNumeric;
import org.jruby.RubyEnumerator;
import org.jruby.runtime.load.Library;
import org.jruby.runtime.builtin.IRubyObject;
import org.jruby.runtime.Arity;
import org.jruby.runtime.Block;
import org.jruby.runtime.ObjectAllocator;
import org.jruby.runtime.ThreadContext;
import org.jruby.anno.JRubyClass;
import org.jruby.anno.JRubyModule;
import org.jruby.anno.JRubyMethod;
import org.jruby.util.IOInputStream;

import static org.jruby.runtime.Visibility.*;

import org.msgpack.MessagePack;
import org.msgpack.packer.BufferPacker;
import org.msgpack.packer.Packer;
import org.msgpack.unpacker.MessagePackBufferUnpacker;
import org.msgpack.unpacker.MessagePackUnpacker;
import org.msgpack.unpacker.UnpackerIterator;
import org.msgpack.type.Value;
import org.msgpack.io.Input;
import org.msgpack.io.LinkedBufferInput;
import org.msgpack.io.StreamInput;


public class MessagePackLibrary implements Library {
  public void load(Ruby runtime, boolean wrap) throws IOException {
    MessagePack msgPack = new MessagePack();
    RubyModule msgpackModule = runtime.defineModule("MessagePack");
    msgpackModule.defineAnnotatedMethods(MessagePackModule.class);
    RubyClass standardErrorClass = runtime.getStandardError();
    RubyClass unpackErrorClass = msgpackModule.defineClassUnder("UnpackError", standardErrorClass, standardErrorClass.getAllocator());
    RubyClass unpackerClass = msgpackModule.defineClassUnder("Unpacker", runtime.getObject(), new UnpackerAllocator(msgPack));
    unpackerClass.defineAnnotatedMethods(Unpacker.class);
  }

  @JRubyModule(name = "MessagePack")
  public static class MessagePackModule {
    private static MessagePack msgPack = new MessagePack();
    private static RubyObjectPacker packer = new RubyObjectPacker(msgPack);
    private static RubyObjectUnpacker unpacker = new RubyObjectUnpacker(msgPack);
    
    @JRubyMethod(module = true, required = 1, optional = 1, alias = {"dump"})
    public static IRubyObject pack(ThreadContext ctx, IRubyObject recv, IRubyObject[] args) throws IOException {
      RubyHash options = (args.length == 2) ? (RubyHash) args[1] : null;
      return packer.pack(args[0], options);
    }
    
    @JRubyMethod(module = true, required = 1, optional = 1, alias = {"load"})
    public static IRubyObject unpack(ThreadContext ctx, IRubyObject recv, IRubyObject[] args) throws IOException {
      RubyHash options = (args.length == 2) ? (RubyHash) args[1] : null;
      RubyString str = args[0].asString();
      return unpacker.unpack(str, options);
    }
  }

  private static class UnpackerAllocator implements ObjectAllocator {
    private MessagePack msgPack;
      
    public UnpackerAllocator(MessagePack msgPack) {
      this.msgPack = msgPack;
    }
      
    public IRubyObject allocate(Ruby runtime, RubyClass klass) {
      return new Unpacker(runtime, klass, msgPack);
    }
  }

  @JRubyClass(name="MessagePack::Unpacker")
  public static class Unpacker extends RubyObject {
    private MessagePack msgPack;
    private RubyObjectUnpacker rubyObjectUnpacker;
    private MessagePackBufferUnpacker bufferUnpacker;
    private MessagePackUnpacker streamUnpacker;
    private UnpackerIterator unpackerIterator;
    private IRubyObject stream;
    private IRubyObject data;
    private RubyObjectUnpacker.CompiledOptions options;
    
    public Unpacker(Ruby runtime, RubyClass type, MessagePack msgPack) {
      super(runtime, type);
      this.msgPack = msgPack;
      this.rubyObjectUnpacker = new RubyObjectUnpacker(msgPack);
      this.bufferUnpacker = null;
      this.streamUnpacker = null;
      this.stream = null;
      this.data = null;
    }

    @JRubyMethod(name = "initialize", optional = 2, visibility = PRIVATE)
    public IRubyObject initialize(ThreadContext ctx, IRubyObject[] args) {
      if (args.length == 0) {
        options = new RubyObjectUnpacker.CompiledOptions(ctx.getRuntime());
      } else if (args.length == 1 && args[0] instanceof RubyHash) {
        options = new RubyObjectUnpacker.CompiledOptions(ctx.getRuntime(), (RubyHash) args[0]);
      } else if (args.length > 0) {
        setStream(ctx, args[0]);
        if (args.length > 2) {
          options = new RubyObjectUnpacker.CompiledOptions(ctx.getRuntime(), (RubyHash) args[1]);
        } else {
          options = new RubyObjectUnpacker.CompiledOptions(ctx.getRuntime());
        }
      }
      return this;
    }

    @JRubyMethod(required = 2)
    public IRubyObject execute(ThreadContext ctx, IRubyObject data, IRubyObject offset) {
      return executeLimit(ctx, data, offset, null);
    }

    @JRubyMethod(name = "execute_limit", required = 3)
    public IRubyObject executeLimit(ThreadContext ctx, IRubyObject data, IRubyObject offset, IRubyObject limit) {
      this.data = null;
      try {
        int jOffset = RubyNumeric.fix2int(offset);
        int jLimit = -1;
        if (limit != null) {
          jLimit = RubyNumeric.fix2int(limit);
        }
        byte[] bytes = data.asString().getBytes();
        MessagePackBufferUnpacker localBufferUnpacker = new MessagePackBufferUnpacker(msgPack, bytes.length);
        localBufferUnpacker.wrap(bytes, jOffset, jLimit == -1 ? bytes.length - jOffset : jLimit);
        this.data = rubyObjectUnpacker.valueToRubyObject(ctx.getRuntime(), localBufferUnpacker.readValue(), options);
        return ctx.getRuntime().newFixnum(jOffset + localBufferUnpacker.getReadByteCount());
      } catch (IOException ioe) {
        // TODO: how to throw Ruby exceptions?
        return ctx.getRuntime().getNil();
      }
    }

    @JRubyMethod(name = "data")
    public IRubyObject getData(ThreadContext ctx) {
      if (data == null) {
        return ctx.getRuntime().getNil();
      } else {
        return data;
      }
    }

    @JRubyMethod(name = "finished?")
    public IRubyObject finished_q(ThreadContext ctx) {
      return data == null ? ctx.getRuntime().getFalse() : ctx.getRuntime().getTrue();
    }

    @JRubyMethod(required = 1)
    public IRubyObject feed(ThreadContext ctx, IRubyObject data) {
      streamUnpacker = null;
      byte[] bytes = data.asString().getBytes();
      if (bufferUnpacker == null) {
        bufferUnpacker = new MessagePackBufferUnpacker(msgPack);
        unpackerIterator = bufferUnpacker.iterator();
      }
      bufferUnpacker.feed(bytes);
      return ctx.getRuntime().getNil();
    }

    @JRubyMethod(name = "feed_each", required = 1)
    public IRubyObject feedEach(ThreadContext ctx, IRubyObject data, Block block) {
      feed(ctx, data);
      each(ctx, block);
      return ctx.getRuntime().getNil();
    }
    
    @JRubyMethod
    public IRubyObject each(ThreadContext ctx, Block block) {
      MessagePackUnpacker localUnpacker = null;
      if (bufferUnpacker == null && streamUnpacker != null) {
        localUnpacker = streamUnpacker;
      } else if (bufferUnpacker != null) {
        localUnpacker = bufferUnpacker;
      } else {
        return ctx.getRuntime().getNil();
      }
      if (block.isGiven()) {
        while (unpackerIterator.hasNext()) {
          Value value = unpackerIterator.next();
          IRubyObject rubyObject = rubyObjectUnpacker.valueToRubyObject(ctx.getRuntime(), value, options);
          block.yield(ctx, rubyObject);
        }
        return ctx.getRuntime().getNil();
      } else {
        return callMethod(ctx, "to_enum");
      }
    }

    @JRubyMethod
    public IRubyObject fill(ThreadContext ctx) {
      return ctx.getRuntime().getNil();
    }

    @JRubyMethod
    public IRubyObject reset(ThreadContext ctx) {
      if (bufferUnpacker != null) {
        bufferUnpacker.reset();
      }
      if (streamUnpacker != null) {
        streamUnpacker.reset();
      }
      return ctx.getRuntime().getNil();
    }

    @JRubyMethod(name = "stream")
    public IRubyObject getStream(ThreadContext ctx) {
      if (stream == null) {
        return ctx.getRuntime().getNil();
      } else {
        return stream;
      }
    }

    @JRubyMethod(name = "stream=", required = 1)
    public IRubyObject setStream(ThreadContext ctx, IRubyObject stream) {
      bufferUnpacker = null;
      this.stream = stream;
      if (stream instanceof RubyStringIO) {
        // TODO: RubyStringIO returns negative numbers when read through IOInputStream#read
        IRubyObject str = ((RubyStringIO) stream).string();
        byte[] bytes = ((RubyString) str).getBytes();
        streamUnpacker = new MessagePackUnpacker(msgPack, new ByteArrayInputStream(bytes));
      } else {
        streamUnpacker = new MessagePackUnpacker(msgPack, new IOInputStream(stream));
      }
      unpackerIterator = streamUnpacker.iterator();
      return getStream(ctx);
    }
  }
}
