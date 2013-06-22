require 'spec_helper'

describe MessagePack do
  it 'unpacks to the original data' do
    data = MessagePack::Extended.new(1, " " * (1 << 16))
    packed = MessagePack.pack(data)

    expect(MessagePack.unpack(packed))
      .to eql data
  end
end
