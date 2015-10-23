# encoding: ascii-8bit
require 'spec_helper'

require 'stringio'
if defined?(Encoding)
  Encoding.default_external = 'ASCII-8BIT'
end

RSpec.describe MessagePack::Packer do
  let :packer do
    MessagePack::Packer.new
  end

  it 'initialize' do
    MessagePack::Packer.new
    MessagePack::Packer.new(nil)
    MessagePack::Packer.new(StringIO.new)
    MessagePack::Packer.new({})
    MessagePack::Packer.new(StringIO.new, {})
  end

  it 'gets options to specify how to pack values' do
    u1 = MessagePack::Packer.new
    expect(u1.compatibility_mode?).to eq false

    u2 = MessagePack::Packer.new(compatibility_mode: true)
    expect(u2.compatibility_mode?).to eq true
  end

  it 'write' do
    packer.write([])
    expect(packer.to_s).to eq "\x90"
  end

  it 'write_nil' do
    packer.write_nil
    expect(packer.to_s).to eq "\xc0"
  end

  it 'write_array_header 0' do
    packer.write_array_header(0)
    expect(packer.to_s).to eq "\x90"
  end

  it 'write_array_header 1' do
    packer.write_array_header(1)
    expect(packer.to_s).to eq "\x91"
  end

  it 'write_map_header 0' do
    packer.write_map_header(0)
    expect(packer.to_s).to eq "\x80"
  end

  it 'write_map_header 1' do
    packer.write_map_header(1)
    expect(packer.to_s).to eq "\x81"
  end

  it 'flush' do
    io = StringIO.new
    pk = MessagePack::Packer.new(io)
    pk.write_nil
    pk.flush
    expect(pk.to_s).to eq ''
    expect(io.string).to eq "\xc0"
  end

  it 'to_msgpack returns String' do
    expect(nil.to_msgpack.class).to eq String
    expect(true.to_msgpack.class).to eq String
    expect(false.to_msgpack.class).to eq String
    expect(1.to_msgpack.class).to eq String
    expect(1.0.to_msgpack.class).to eq String
    expect("".to_msgpack.class).to eq String
    expect(Hash.new.to_msgpack.class).to eq String
    expect(Array.new.to_msgpack.class).to eq String
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
    expect(MessagePack.pack(CustomPack01.new)).to eq [1,2].to_msgpack
    expect(MessagePack.pack(CustomPack02.new)).to eq [1,2].to_msgpack
    expect(CustomPack01.new.to_msgpack).to eq [1,2].to_msgpack
    expect(CustomPack02.new.to_msgpack).to eq [1,2].to_msgpack
  end

  it 'calls custom to_msgpack method with io' do
    s01 = StringIO.new
    MessagePack.pack(CustomPack01.new, s01)
    expect(s01.string).to eq [1,2].to_msgpack

    s02 = StringIO.new
    MessagePack.pack(CustomPack02.new, s02)
    expect(s02.string).to eq [1,2].to_msgpack

    s03 = StringIO.new
    CustomPack01.new.to_msgpack(s03)
    expect(s03.string).to eq [1,2].to_msgpack

    s04 = StringIO.new
    CustomPack02.new.to_msgpack(s04)
    expect(s04.string).to eq [1,2].to_msgpack
  end

  context 'in compatibility mode' do
    it 'does not use the bin types' do
      packed = MessagePack.pack('hello'.force_encoding(Encoding::BINARY), compatibility_mode: true)
      expect(packed).to eq("\xA5hello")
      packed = MessagePack.pack(('hello' * 100).force_encoding(Encoding::BINARY), compatibility_mode: true)
      expect(packed).to start_with("\xDA\x01\xF4")

      packer = MessagePack::Packer.new(compatibility_mode: 1)
      packed = packer.pack(('hello' * 100).force_encoding(Encoding::BINARY))
      expect(packed.to_str).to start_with("\xDA\x01\xF4")
    end

    it 'does not use the str8 type' do
      packed = MessagePack.pack('x' * 32, compatibility_mode: true)
      expect(packed).to start_with("\xDA\x00\x20")
    end
  end

  class ValueOne
    def initialize(num)
      @num = num
    end
    def num
      @num
    end
    def to_msgpack_ext
      @num.to_msgpack
    end
    def self.from_msgpack_ext(data)
      self.new(MessagePack.unpack(data))
    end
  end

  class ValueTwo
    def initialize(num)
      @num_s = num.to_s
    end
    def num
      @num_s.to_i
    end
    def to_msgpack_ext
      @num_s.to_msgpack
    end
    def self.from_msgpack_ext(data)
      self.new(MessagePack.unpack(data))
    end
  end

  describe '#type_registered?' do
    it 'receive Class or Integer, and return bool' do
      expect(subject.type_registered?(0x00)).to be_falsy
      expect(subject.type_registered?(0x01)).to be_falsy
      expect(subject.type_registered?(::ValueOne)).to be_falsy
    end

    it 'returns true if specified type or class is already registered' do
      subject.register_type(0x30, ::ValueOne, :to_msgpack_ext)
      subject.register_type(0x31, ::ValueTwo, :to_msgpack_ext)

      expect(subject.type_registered?(0x00)).to be_falsy
      expect(subject.type_registered?(0x01)).to be_falsy

      expect(subject.type_registered?(0x30)).to be_truthy
      expect(subject.type_registered?(0x31)).to be_truthy
      expect(subject.type_registered?(::ValueOne)).to be_truthy
      expect(subject.type_registered?(::ValueTwo)).to be_truthy
    end
  end

  describe '#register_type' do
    it 'get type and class mapping for packing' do
      packer = MessagePack::Packer.new
      packer.register_type(0x01, ValueOne){|obj| obj.to_msgpack_ext }
      packer.register_type(0x02, ValueTwo){|obj| obj.to_msgpack_ext }

      packer = MessagePack::Packer.new
      packer.register_type(0x01, ValueOne, :to_msgpack_ext)
      packer.register_type(0x02, ValueTwo, :to_msgpack_ext)

      packer = MessagePack::Packer.new
      packer.register_type(0x01, ValueOne, &:to_msgpack_ext)
      packer.register_type(0x02, ValueTwo, &:to_msgpack_ext)
    end

    it 'returns a Hash which contains map of Class and type' do
      packer = MessagePack::Packer.new
      packer.register_type(0x01, ValueOne, :to_msgpack_ext)
      packer.register_type(0x02, ValueTwo, :to_msgpack_ext)

      expect(packer.registered_types).to be_a(Array)
      expect(packer.registered_types.size).to eq(2)

      one = packer.registered_types[0]
      expect(one.keys.sort).to eq([:type, :class, :packer].sort)
      expect(one[:type]).to eq(0x01)
      expect(one[:class]).to eq(ValueOne)
      expect(one[:packer]).to eq(:to_msgpack_ext)

      two = packer.registered_types[1]
      expect(two.keys.sort).to eq([:type, :class, :packer].sort)
      expect(two[:type]).to eq(0x02)
      expect(two[:class]).to eq(ValueTwo)
      expect(two[:packer]).to eq(:to_msgpack_ext)
    end
  end
end
