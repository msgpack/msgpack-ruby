# encoding: ascii-8bit
require 'spec_helper'

describe Factory do
  let :factory do
    Factory.new
  end

  it 'packer' do
    factory.packer.should be_kind_of(Packer)
  end

  it 'unpacker' do
    factory.unpacker.should be_kind_of(Unpacker)
  end

  class MyType
    def initialize(string)
      @string = string
    end

    attr_reader :string

    def to_msgpack_ext
      [@string].pack('m')
    end

    def self.from_msgpack_ext(data)
      MyType.new(data.unpack('m')[0])
    end
  end

  it 'register_type' do
    factory.register_type(0x7f, MyType)
    factory.register_type(0x7f, MyType,
                          packer: :to_msgpack_ext,
                          unpacker: :from_msgpack_ext)
    factory.register_type(0x7f, MyType,
                          packer: lambda {|my| [my.string].pack('m') },
                          unpacker: lambda {|data| MyType.new(data.unpack('m')[0]) })
  end

  it 'packer register_type' do
    factory.packer.register_type(0x7f, MyType) {|my| [my.string].pack('m') }
    factory.packer.register_type(0x7f, MyType, :to_msgpack_ext)
  end

  it 'unpacker register_type' do
    factory.unpacker.register_type(0x7f) {|data| MyType.new(data.unpack('m')[0]) }
    factory.unpacker.register_type(0x7f, MyType, :from_msgpack_ext)
  end
end
