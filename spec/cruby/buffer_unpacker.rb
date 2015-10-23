# encoding: ascii-8bit
require 'spec_helper'

RSpec.describe Unpacker do
  let :unpacker do
    Unpacker.new
  end

  let :packer do
    Packer.new
  end

  it 'buffer' do
    o1 = unpacker.buffer.object_id
    unpacker.buffer << 'frsyuki'
    expect(unpacker.buffer.to_s).to eq 'frsyuki'
    expect(unpacker.buffer.object_id).to eq o1
  end
end
