# frozen_string_literal: true

require 'spec_helper'

describe MessagePack::Timestamp do
  # describe '#dump and #load' do
  #   it 'is alias as pack and unpack' do
  #     packed = subject.pack(time)
  #     expect(subject.unpack(packed)).to be == time
  #   end
  # end

  describe 'timestamp32' do
    it 'handles [1, 0]' do
      t = MessagePack::Timestamp.new(1, 0)

      payload = t.to_msgpack_ext
      unpacked = MessagePack::Timestamp.from_msgpack_ext(payload)

      expect(unpacked).to eq(t)
    end
  end

  describe 'timestamp64' do
    it 'handles [1, 1]' do
      t = MessagePack::Timestamp.new(1, 1)

      payload = t.to_msgpack_ext
      unpacked = MessagePack::Timestamp.from_msgpack_ext(payload)

      expect(unpacked).to eq(t)
    end
  end

  describe 'timestamp96' do
    it 'handles [-1, 0]' do
      t = MessagePack::Timestamp.new(-1, 0)

      payload = t.to_msgpack_ext
      unpacked = MessagePack::Timestamp.from_msgpack_ext(payload)

      expect(unpacked).to eq(t)
    end

    it 'handles [-1, 999_999_999]' do
      t = MessagePack::Timestamp.new(-1, 999_999_999)

      payload = t.to_msgpack_ext
      unpacked = MessagePack::Timestamp.from_msgpack_ext(payload)

      expect(unpacked).to eq(t)
    end
  end

  describe 'factory' do
    let(:time) { Time.now }

    it 'serializes and deserializes Time with DefaultFactory' do
      packed = MessagePack.pack(time)
      unpacked = MessagePack.unpack(packed)

      expect(unpacked).to eq(time)
    end
  end
end
