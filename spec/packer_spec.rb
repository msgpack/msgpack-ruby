require 'spec_helper'

describe MessagePack::Packer do
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
