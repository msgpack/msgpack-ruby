# encoding: ascii-8bit
require 'spec_helper'

eval("return") if java?

describe MessagePack::Factory do
  subject do
    described_class.new
  end

  describe '#packer' do
    it 'creates a Packer instance' do
      subject.packer.should be_kind_of(MessagePack::Packer)
    end

    it 'creates new instance' do
      subject.packer.object_id.should_not == subject.packer.object_id
    end
  end

  describe '#unpacker' do
    it 'creates a Unpacker instance' do
      subject.unpacker.should be_kind_of(MessagePack::Unpacker)
    end

    it 'creates new instance' do
      subject.unpacker.object_id.should_not == subject.unpacker.object_id
    end

    it 'creates unpacker with symbolize_keys option' do
      unpacker = subject.unpacker(symbolize_keys: true)
      unpacker.feed(MessagePack.pack({'k'=>'v'}))
      unpacker.read.should == {:k => 'v'}
    end

    it 'creates unpacker with allow_unknown_ext option' do
      unpacker = subject.unpacker(allow_unknown_ext: true)
      unpacker.feed(MessagePack::ExtensionValue.new(1, 'a').to_msgpack)
      unpacker.read.should == MessagePack::ExtensionValue.new(1, 'a')
    end
  end

  class MyType
    def initialize(a, b)
      @a = a
      @b = b
    end

    attr_reader :a, :b

    def to_msgpack_ext
      [a, b].pack('CC')
    end

    def self.from_msgpack_ext(data)
      new(*data.unpack('CC'))
    end

    def to_msgpack_ext_only_a
      [a, 0].pack('CC')
    end

    def self.from_msgpack_ext_only_b(data)
      a, b = *data.unpack('CC')
      new(0, b)
    end
  end

  describe '#register_type' do
    let :src do
      ::MyType.new(1, 2)
    end

    it 'registers #to_msgpack_ext and .from_msgpack_ext by default' do
      subject.register_type(0x7f, ::MyType)

      data = subject.packer.write(src).to_s
      my = subject.unpacker.feed(data).read
      my.a.should == 1
      my.b.should == 2
    end

    it 'registers custom packer method name' do
      subject.register_type(0x7f, ::MyType, packer: :to_msgpack_ext_only_a, unpacker: :from_msgpack_ext)

      data = subject.packer.write(src).to_s
      my = subject.unpacker.feed(data).read
      my.a.should == 1
      my.b.should == 0
    end

    it 'registers custom unpacker method name' do
      subject.register_type(0x7f, ::MyType, packer: :to_msgpack_ext, unpacker: 'from_msgpack_ext_only_b')

      data = subject.packer.write(src).to_s
      my = subject.unpacker.feed(data).read
      my.a.should == 0
      my.b.should == 2
    end

    it 'registers custom proc objects' do
      pk = lambda {|obj| [obj.a + obj.b].pack('C') }
      uk = lambda {|data| ::MyType.new(data.unpack('C').first, -1) }
      subject.register_type(0x7f, ::MyType, packer: pk, unpacker: uk)

      data = subject.packer.write(src).to_s
      my = subject.unpacker.feed(data).read
      my.a.should == 3
      my.b.should == -1
    end

    it 'does not affect existent packer and unpackers' do
      subject.register_type(0x7f, ::MyType)
      packer = subject.packer
      unpacker = subject.unpacker

      subject.register_type(0x7f, ::MyType, packer: :to_msgpack_ext_only_a, unpacker: :from_msgpack_ext_only_b)

      data = packer.write(src).to_s
      my = unpacker.feed(data).read
      my.a.should == 1
      my.b.should == 2
    end
  end

  describe 'DefaultFactory' do
    it 'is a factory' do
      MessagePack::DefaultFactory.should be_kind_of(MessagePack::Factory)
    end
  end
end
