require 'spec_helper'

class CustomMessagePackExtension; end

class UnregisteredCustomMessagePackExtension; end

class Z; end

describe MessagePack do
  before(:all) do
    MessagePack.register_type CustomMessagePackExtension
    MessagePack.register_type Z
  end

  it 'serializes custom types' do
    o = CustomMessagePackExtension.new
    expect {
      MessagePack.pack(o).should_not be_nil
    }.to_not raise_error
  end

  it 'should raise for unregistered types' do
    expect {
      MessagePack.pack(UnregisteredCustomMessagePackExtension.new)
    }.to raise_error
  end

  it 'should unpack a custom type' do
    o = CustomMessagePackExtension.new
    expect {
      MessagePack.unpack(MessagePack.pack(o))
    }.to_not raise_error
  end

  it 'should unpack a fixext type' do
    z = Z.new
    packed = MessagePack.pack(z)
    expect(packed.size).to be == 18 # fixext 16
  end
end
