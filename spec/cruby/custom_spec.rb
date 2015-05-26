require 'spec_helper'

class CustomMessagePackExtension; end

class UnregisteredCustomMessagePackExtension; end

class Z; end

class CustomMessagePackExtensionWithSerializer
  class << self
    attr_accessor :serialized, :deserialized

    def reset
      @deserialized = @serialized = false
    end
  end

  def to_msgpack
    self.class.serialized = true
    "1"
  end

  def self.from_msgpack(data)
    @deserialized = data
    self.new
  end
end

class CustomMessagePackExtensionWithSerializerNoFrom
  def to_msgpack; end
end

describe MessagePack do
  before(:all) do
    MessagePack.register_type CustomMessagePackExtension
    MessagePack.register_type Z
    MessagePack.register_type CustomMessagePackExtensionWithSerializer
  end

  before(:each) do
    CustomMessagePackExtensionWithSerializer.reset
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

  it 'should support custom serialization' do
    expect(CustomMessagePackExtensionWithSerializer.serialized).to be false
    expect(CustomMessagePackExtensionWithSerializer.deserialized).to be false

    packed = MessagePack.pack(CustomMessagePackExtensionWithSerializer.new)
    expect(CustomMessagePackExtensionWithSerializer.serialized).to be true

    obj = MessagePack.unpack(packed)
    expect(CustomMessagePackExtensionWithSerializer.deserialized).to be == "1"
    expect(obj.class).to be CustomMessagePackExtensionWithSerializer
  end

  it 'should support custom types in collections' do
    a = [ CustomMessagePackExtension.new ]
    packed = MessagePack.pack(a)
    a = MessagePack.unpack(packed)
    expect(a[0].class).to be CustomMessagePackExtension
  end

  it 'should raise when a custom serializer does not implement from_msgpack' do
    expect {
      MessagePack.register_type CustomMessagePackExtensionWithSerializerNoFrom
    }.to raise_error
  end
end
