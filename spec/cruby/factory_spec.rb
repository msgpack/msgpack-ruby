# encoding: ascii-8bit
require 'spec_helper'

describe Factory do
  let :factory do
    Factory.new
  end

  let :packer do
    factory.packer
  end

  let :unpacker do
    factory.unpacker
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
    packer.register_type(0x7f, MyType) {|my| [my.string].pack('m') }
    packer.register_type(0x7f, MyType, :to_msgpack_ext)
    packer.write(MyType.new("abc"))
    packer.buffer.to_s.should == "\xc7\x05\x7fYWJj\n"
  end

  it 'unpacker register_type' do
    unpacker.register_type(0x7f) {|data| MyType.new(data.unpack('m')[0]) }
    unpacker.register_type(0x7f, MyType, :from_msgpack_ext)
    packer.write_ext(0x7f, ["abc"].pack('m'))
    unpacker.feed(packer.buffer)
    my = unpacker.read
    my.class.should == MyType
    my.string.should == "abc"
  end

  it 'allow_unknown_ext' do
    factory.unpacker(allow_unknown_ext: true)
  end
end
