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

  it 'write_ext_header 1 1' do
    packer.write_ext_header(1, 1)
    packer.to_s.should == "\xd4\x01"
  end

  it 'write_ext_header 2 1' do
    packer.write_ext_header(2, 1)
    packer.to_s.should == "\xd5\x01"
  end

  it 'write_ext_header 4 1' do
    packer.write_ext_header(4, 1)
    packer.to_s.should == "\xd6\x01"
  end

  it 'write_ext_header 8 1' do
    packer.write_ext_header(8, 1)
    packer.to_s.should == "\xd7\x01"
  end

  it 'write_ext_header 16 1' do
    packer.write_ext_header(16, 1)
    packer.to_s.should == "\xd8\x01"
  end

  # (2^8) - 1 bytes
  it 'write_ext_header ((1 << 8) -1), 1' do
    packer.write_ext_header((1 << 8) - 1, 1)
    packer.to_s.should == "\xc7\xff\x01"
  end

  # (2^8)
  it 'write_ext_header ((1 << 8), 1' do
    packer.write_ext_header((1 << 8), 1)
    packer.to_s.should == "\xc8\x01\x00\x01"
  end

  # (2^16) - 1 bytes
  it 'write_ext_header ((1 << 16) -1), 1' do
    packer.write_ext_header((1 << 16) - 1, 1)
    packer.to_s.should == "\xc8\xff\xff\x01"
  end

  # 2^16 bytes
  it 'write_ext_header 1 << 16, 1' do
    packer.write_ext_header((1 << 16), 1)
    packer.to_s.should == "\xc9\x00\x01\x00\x00\x01"
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
    MessagePack::Extended.new(1, "").to_msgpack.class.should == String
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
end

