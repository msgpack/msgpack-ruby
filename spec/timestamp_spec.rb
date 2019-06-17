# frozen_string_literal
require 'spec_helper'

describe MessagePack::Timestamp do
  # describe '#dump and #load' do
  #   it 'is alias as pack and unpack' do
  #     packed = subject.pack(time)
  #     expect(subject.unpack(packed)).to be == time
  #   end
  # end

  describe 'timestamp32' do
    it "serializes and deserializes MessagePack::Timestamp" do
      t = MessagePack::Timestamp.new(1, 0)

      payload = t.to_msgpack_payload
      unpacked = MessagePack::Timestamp.from_payload(payload)

      expect(unpacked).to eq(t)
    end
  end

  describe 'timestamp64' do
    it "serializes and deserializes MessagePack::Timestamp" do
      t = MessagePack::Timestamp.new(1, 1)

      payload = t.to_msgpack_payload
      unpacked = MessagePack::Timestamp.from_payload(payload)

      expect(unpacked).to eq(t)
    end
  end

  describe 'timestamp96' do
    it "serializes and deserializes MessagePack::Timestamp" do
      t = MessagePack::Timestamp.new(-1, 0)

      payload = t.to_msgpack_payload
      unpacked = MessagePack::Timestamp.from_payload(payload)

      expect(unpacked).to eq(t)
    end
  end
end
