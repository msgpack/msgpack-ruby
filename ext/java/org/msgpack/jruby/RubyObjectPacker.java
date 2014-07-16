package org.msgpack.jruby;


import java.io.IOException;

import org.jcodings.Encoding;

import org.msgpack.MessagePack;
import org.msgpack.packer.BufferPacker;

import org.jruby.Ruby;
import org.jruby.RubyObject;
import org.jruby.RubyNil;
import org.jruby.RubyBoolean;
import org.jruby.RubyBignum;
import org.jruby.RubyInteger;
import org.jruby.RubyFixnum;
import org.jruby.RubyFloat;
import org.jruby.RubyString;
import org.jruby.RubySymbol;
import org.jruby.RubyArray;
import org.jruby.RubyHash;
import org.jruby.RubyEncoding;
import org.jruby.runtime.builtin.IRubyObject;
import org.jruby.runtime.encoding.EncodingService;


public class RubyObjectPacker {
  private final MessagePack msgPack;

  public RubyObjectPacker(MessagePack msgPack) {
    this.msgPack = msgPack;
  }

  static class CompiledOptions {
    public final Encoding encoding;

    public CompiledOptions(Ruby runtime, RubyHash options) {
      EncodingService encodingService = runtime.getEncodingService();
      Encoding externalEncoding = null;
      if (options != null) {
        IRubyObject rubyEncoding = options.fastARef(runtime.newSymbol("encoding"));
        externalEncoding = encodingService.getEncodingFromObject(rubyEncoding);
      }
      if (externalEncoding == null) {
        externalEncoding = runtime.getDefaultExternalEncoding();
      }
      encoding = (externalEncoding != encodingService.getAscii8bitEncoding()) ? externalEncoding : null;
    }
  }

  public RubyString pack(IRubyObject o, RubyHash options) throws IOException {
    return RubyString.newStringNoCopy(o.getRuntime(), packRaw(o, new CompiledOptions(o.getRuntime(), options)));
  }

  @Deprecated
  public byte[] packRaw(IRubyObject o) throws IOException {
    if (o == null) {
      return new byte[] { -64 };
    } else {
      return packRaw(o.getRuntime(), o);
    }
  }

  public byte[] packRaw(Ruby runtime, IRubyObject o) throws IOException {
    return packRaw(runtime, o, null);
  }

  public byte[] packRaw(Ruby runtime, IRubyObject o, RubyHash options) throws IOException {
    return packRaw(o, new CompiledOptions(runtime, options));
  }

  byte[] packRaw(IRubyObject o, CompiledOptions options) throws IOException {
    BufferPacker packer = msgPack.createBufferPacker();
    write(packer, o, options);
    return packer.toByteArray();
  }

  private void write(BufferPacker packer, IRubyObject o, CompiledOptions options) throws IOException {
    if (o == null || o instanceof RubyNil) {
      packer.writeNil();
    } else if (o instanceof RubyBoolean) {
      packer.write(((RubyBoolean) o).isTrue());
    } else if (o instanceof RubyBignum) {
      write(packer, (RubyBignum) o);
    } else if (o instanceof RubyInteger) {
      write(packer, (RubyInteger) o);
    } else if (o instanceof RubyFixnum) {
      write(packer, (RubyFixnum) o);
    } else if (o instanceof RubyFloat) {
      write(packer, (RubyFloat) o);
    } else if (o instanceof RubyString) {
      write(packer, (RubyString) o, options);
    } else if (o instanceof RubySymbol) {
      write(packer, (RubySymbol) o, options);
    } else if (o instanceof RubyArray) {
      write(packer, (RubyArray) o, options);
    } else if (o instanceof RubyHash) {
      write(packer, (RubyHash) o, options);
    } else {
      throw o.getRuntime().newArgumentError(String.format("Cannot pack type: %s", o.getClass().getName()));
    }
  }

  private void write(BufferPacker packer, RubyBignum bignum) throws IOException {
    packer.write(bignum.getBigIntegerValue());
  }

  private void write(BufferPacker packer, RubyInteger integer) throws IOException {
    packer.write(integer.getLongValue());
  }

  private void write(BufferPacker packer, RubyFixnum fixnum) throws IOException {
    packer.write(fixnum.getLongValue());
  }

  private void write(BufferPacker packer, RubyFloat flt) throws IOException {
    packer.write(flt.getDoubleValue());
  }

  private void write(BufferPacker packer, RubyString str, CompiledOptions options) throws IOException {
    if ((options.encoding != null) && (str.getEncoding() != options.encoding)) {
      Ruby runtime = str.getRuntime();
      str = (RubyString) str.encode(runtime.getCurrentContext(), RubyEncoding.newEncoding(runtime, options.encoding));
    }
    packer.write(str.getBytes());
  }

  private void write(BufferPacker packer, RubySymbol sym, CompiledOptions options) throws IOException {
    write(packer, sym.asString(), options);
  }

  private void write(BufferPacker packer, RubyArray array, CompiledOptions options) throws IOException {
    int count = array.size();
    packer.writeArrayBegin(count);
    for (int i = 0; i < count; i++) {
      write(packer, (RubyObject) array.entry(i), options);
    }
    packer.writeArrayEnd();
  }

  private void write(BufferPacker packer, RubyHash hash, CompiledOptions options) throws IOException {
    int count = hash.size();
    packer.writeMapBegin(count);
    RubyArray keys = hash.keys();
    RubyArray values = hash.rb_values();
    for (int i = 0; i < count; i++) {
      write(packer, (RubyObject) keys.entry(i), options);
      write(packer, (RubyObject) values.entry(i), options);
    }
    packer.writeMapEnd();
  }
}
