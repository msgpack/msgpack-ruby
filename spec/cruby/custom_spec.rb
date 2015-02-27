require 'spec_helper'

class CustomMessagePackExtension; end

class UnregisteredCustomMessagePackExtension; end

describe MessagePack do
  before(:all) do
    MessagePack.register_type CustomMessagePackExtension
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
end
