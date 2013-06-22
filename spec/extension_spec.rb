require 'spec_helper'

describe MessagePack do
  it 'unpacks to the original data' do
    data = MessagePack::Extended.new(1, " " * (1 << 16))
    packed = MessagePack.pack(data)

    expect(MessagePack.unpack(packed))
      .to eql data
  end

  it "raises an error when type type is out of range" do
    expect { MessagePack::Extended.new(128, "") }
      .to raise_error

    expect { MessagePack::Extended.new(-129, "") }
      .to raise_error
  end
end
