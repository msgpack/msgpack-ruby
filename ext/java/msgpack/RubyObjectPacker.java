package msgpack;

import java.io.IOException;

import org.msgpack.MessagePack;
import org.msgpack.packer.Packer;
import org.msgpack.packer.MessagePackBufferPacker;

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
import org.jruby.runtime.builtin.IRubyObject;


public class RubyObjectPacker extends MessagePackBufferPacker {
  private final Packer packer;

  public RubyObjectPacker(MessagePack msgPack, Packer packer) {
    super(msgPack);
    this.packer = packer;
  }

  public Packer write(IRubyObject o) throws IOException {
    if (o == null || o instanceof RubyNil) {
      packer.writeNil();
      return this;
    } else if (o instanceof RubyBoolean) {
      packer.write(((RubyBoolean) o).isTrue());
      return this;
    } else if (o instanceof RubyBignum) {
      return write((RubyBignum) o);
    } else if (o instanceof RubyInteger) {
      return write((RubyInteger) o);
    } else if (o instanceof RubyFixnum) {
      return write((RubyFixnum) o);
    } else if (o instanceof RubyFloat) {
      return write((RubyFloat) o);
    } else if (o instanceof RubyString) {
      return write((RubyString) o);
    } else if (o instanceof RubySymbol) {
      return write((RubySymbol) o);
    } else if (o instanceof RubyArray) {
      return write((RubyArray) o);
    } else if (o instanceof RubyHash) {
      return write((RubyHash) o);
    } else {
      throw o.getRuntime().newArgumentError(String.format("Cannot pack type: %s", o.getClass().getName()));
    }
  }

  public final Packer write(RubyBignum bignum) throws IOException {
    packer.write(bignum.getBigIntegerValue());
    return this;
  }

  public final Packer write(RubyInteger integer) throws IOException {
    packer.write(integer.getLongValue());
    return this;
  }

  public final Packer write(RubyFixnum fixnum) throws IOException {
    packer.write(fixnum.getLongValue());
    return this;
  }

  public final Packer write(RubyFloat flt) throws IOException {
    packer.write(flt.getDoubleValue());
    return this;
  }

  public final Packer write(RubyString str) throws IOException {
    packer.write(str.getBytes());
    return this;
  }

  public final Packer write(RubySymbol sym) throws IOException {
    return write((RubyString) sym.to_s());
  }

  public final Packer write(RubyArray array) throws IOException {
    int count = array.size();
    packer.writeArrayBegin(count);
    for (int i = 0; i < count; i++) {
      write((RubyObject) array.entry(i));
    }
    packer.writeArrayEnd();
    return this;
  }

  public final Packer write(RubyHash hash) throws IOException {
    int count = hash.size();
    packer.writeMapBegin(count);
    RubyArray keys = hash.keys();
    RubyArray values = hash.rb_values();
    for (int i = 0; i < count; i++) {
      write((RubyObject) keys.entry(i));
      write((RubyObject) values.entry(i));
    }
    packer.writeMapEnd();
    return this;
  }
}
