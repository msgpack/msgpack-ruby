# encoding: ascii-8bit
require 'spec_helper'

require 'stringio'
if defined?(Encoding)
  Encoding.default_external = 'ASCII-8BIT'
end

RSpec.describe MessagePack do
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
end
