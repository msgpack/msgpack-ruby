# encoding: ascii-8bit
require 'spec_helper'

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

    it 'creates unpacker without allow_unknown_ext option' do
      unpacker = subject.unpacker
      unpacker.feed(MessagePack::ExtensionValue.new(1, 'a').to_msgpack)
      expect{ unpacker.read }.to raise_error(MessagePack::UnknownExtTypeError)
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

  class MyType2 < MyType
  end

  describe '#registered_types' do
    it 'returns Array' do
      expect(subject.registered_types).to be_instance_of(Array)
    end

    it 'returns Array of Hash contains :type, :class, :packer, :unpacker' do
      subject.register_type(0x20, ::MyType)
      subject.register_type(0x21, ::MyType2)

      list = subject.registered_types

      expect(list.size).to eq(2)
      expect(list[0]).to be_instance_of(Hash)
      expect(list[1]).to be_instance_of(Hash)
      expect(list[0].keys.sort).to eq([:type, :class, :packer, :unpacker].sort)
      expect(list[1].keys.sort).to eq([:type, :class, :packer, :unpacker].sort)

      expect(list[0][:type]).to eq(0x20)
      expect(list[0][:class]).to eq(::MyType)
      expect(list[0][:packer]).to eq(:to_msgpack_ext)
      expect(list[0][:unpacker]).to eq(:from_msgpack_ext)

      expect(list[1][:type]).to eq(0x21)
      expect(list[1][:class]).to eq(::MyType2)
      expect(list[1][:packer]).to eq(:to_msgpack_ext)
      expect(list[1][:unpacker]).to eq(:from_msgpack_ext)
    end

    it 'returns Array of Hash which has nil for unregistered feature' do
      subject.register_type(0x21, ::MyType2, unpacker: :from_msgpack_ext)
      subject.register_type(0x20, ::MyType, packer: :to_msgpack_ext)

      list = subject.registered_types

      expect(list.size).to eq(2)
      expect(list[0]).to be_instance_of(Hash)
      expect(list[1]).to be_instance_of(Hash)
      expect(list[0].keys.sort).to eq([:type, :class, :packer, :unpacker].sort)
      expect(list[1].keys.sort).to eq([:type, :class, :packer, :unpacker].sort)

      expect(list[0][:type]).to eq(0x20)
      expect(list[0][:class]).to eq(::MyType)
      expect(list[0][:packer]).to eq(:to_msgpack_ext)
      expect(list[0][:unpacker]).to be_nil

      expect(list[1][:type]).to eq(0x21)
      expect(list[1][:class]).to eq(::MyType2)
      expect(list[1][:packer]).to be_nil
      expect(list[1][:unpacker]).to eq(:from_msgpack_ext)
    end
  end

  describe '#type_registered?' do
    it 'receive Class or Integer, and return bool' do
      expect(subject.type_registered?(0x00)).to be_falsy
      expect(subject.type_registered?(0x01)).to be_falsy
      expect(subject.type_registered?(::MyType)).to be_falsy
    end

    it 'has option to specify what types are registered for' do
      expect(subject.type_registered?(0x00, :both)).to be_falsy
      expect(subject.type_registered?(0x00, :packer)).to be_falsy
      expect(subject.type_registered?(0x00, :unpacker)).to be_falsy
      expect{ subject.type_registered?(0x00, :something) }.to raise_error(ArgumentError)
    end

    it 'returns true if specified type or class is already registered' do
      subject.register_type(0x20, ::MyType)
      subject.register_type(0x21, ::MyType2)

      expect(subject.type_registered?(0x00)).to be_falsy
      expect(subject.type_registered?(0x01)).to be_falsy

      expect(subject.type_registered?(0x20)).to be_truthy
      expect(subject.type_registered?(0x21)).to be_truthy
      expect(subject.type_registered?(::MyType)).to be_truthy
      expect(subject.type_registered?(::MyType2)).to be_truthy
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

    require_relative 'exttypes'

    it 'should be referred by MessagePack.pack and MessagePack.unpack' do
      MessagePack::DefaultFactory.register_type(DummyTimeStamp1::TYPE, DummyTimeStamp1)
      MessagePack::DefaultFactory.register_type(DummyTimeStamp2::TYPE, DummyTimeStamp2, packer: :serialize, unpacker: :deserialize)

      t = Time.now

      dm1 = DummyTimeStamp1.new(t.to_i, t.usec)
      expect(MessagePack.unpack(MessagePack.pack(dm1))).to eq(dm1)

      dm2 = DummyTimeStamp1.new(t.to_i, t.usec)
      expect(MessagePack.unpack(MessagePack.pack(dm2))).to eq(dm2)
    end
  end
end
