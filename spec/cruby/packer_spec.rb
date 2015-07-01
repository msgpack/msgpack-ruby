# encoding: ascii-8bit
require 'spec_helper'

require 'stringio'
if defined?(Encoding)
  Encoding.default_external = 'ASCII-8BIT'
end

describe Packer do
  let :packer do
    Packer.new
  end

  it 'initialize' do
    Packer.new
    Packer.new(nil)
    Packer.new(StringIO.new)
    Packer.new({})
    Packer.new(StringIO.new, {})
  end

  #it 'Packer' do
  #  Packer(packer).object_id.should == packer.object_id
  #  Packer(nil).class.should == Packer
  #  Packer('').class.should == Packer
  #  Packer('initbuf').to_s.should == 'initbuf'
  #end

  it 'write' do
    packer.write([])
    packer.to_s.should == "\x90"
  end

  it 'write_nil' do
    packer.write_nil
    packer.to_s.should == "\xc0"
  end

  it 'write_array_header 0' do
    packer.write_array_header(0)
    packer.to_s.should == "\x90"
  end

  it 'write_array_header 1' do
    packer.write_array_header(1)
    packer.to_s.should == "\x91"
  end

  it 'write_map_header 0' do
    packer.write_map_header(0)
    packer.to_s.should == "\x80"
  end

  it 'write_map_header 1' do
    packer.write_map_header(1)
    packer.to_s.should == "\x81"
  end

  it 'flush' do
    io = StringIO.new
    pk = Packer.new(io)
    pk.write_nil
    pk.flush
    pk.to_s.should == ''
    io.string.should == "\xc0"
  end

  it 'to_msgpack returns String' do
    nil.to_msgpack.class.should == String
    true.to_msgpack.class.should == String
    false.to_msgpack.class.should == String
    1.to_msgpack.class.should == String
    1.0.to_msgpack.class.should == String
    "".to_msgpack.class.should == String
    Hash.new.to_msgpack.class.should == String
    Array.new.to_msgpack.class.should == String
  end

  class CustomPack01
    def to_msgpack(pk=nil)
      return MessagePack.pack(self, pk) unless pk.class == MessagePack::Packer
      pk.write_array_header(2)
      pk.write(1)
      pk.write(2)
      return pk
    end
  end

  class CustomPack02
    def to_msgpack(pk=nil)
      [1,2].to_msgpack(pk)
    end
  end

  it 'calls custom to_msgpack method' do
    MessagePack.pack(CustomPack01.new).should == [1,2].to_msgpack
    MessagePack.pack(CustomPack02.new).should == [1,2].to_msgpack
    CustomPack01.new.to_msgpack.should == [1,2].to_msgpack
    CustomPack02.new.to_msgpack.should == [1,2].to_msgpack
  end

  it 'calls custom to_msgpack method with io' do
    s01 = StringIO.new
    MessagePack.pack(CustomPack01.new, s01)
    s01.string.should == [1,2].to_msgpack

    s02 = StringIO.new
    MessagePack.pack(CustomPack02.new, s02)
    s02.string.should == [1,2].to_msgpack

    s03 = StringIO.new
    CustomPack01.new.to_msgpack(s03)
    s03.string.should == [1,2].to_msgpack

    s04 = StringIO.new
    CustomPack02.new.to_msgpack(s04)
    s04.string.should == [1,2].to_msgpack
  end

  context 'in compatibility mode' do
    it 'does not use the bin types' do
      packed = MessagePack.pack('hello'.force_encoding(Encoding::BINARY), compatibility_mode: true)
      packed.should eq("\xA5hello")
      packed = MessagePack.pack(('hello' * 100).force_encoding(Encoding::BINARY), compatibility_mode: true)
      packed.should start_with("\xDA\x01\xF4")

      packer = MessagePack::Packer.new(compatibility_mode: 1)
      packed = packer.pack(('hello' * 100).force_encoding(Encoding::BINARY))
      packed.to_str.should start_with("\xDA\x01\xF4")
    end

    it 'does not use the str8 type' do
      packed = MessagePack.pack('x' * 32, compatibility_mode: true)
      packed.should start_with("\xDA\x00\x20")
    end
  end
end

