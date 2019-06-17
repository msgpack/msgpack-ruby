# frozen_string_literal: true

require 'spec_helper'

describe MessagePack::Timestamp do

  describe 'register_type with Time' do
    let(:factory) do
      factory = MessagePack::Factory.new
      factory.register_type(MessagePack::Timestamp::TYPE, Time)
      factory
    end

    let(:time) { Time.local(2019, 6, 17, 1, 2, 3, 123_456) }
    it 'serializes and deserializes Time' do
      packed = factory.pack(time)
      unpacked = factory.unpack(packed)
      expect(unpacked).to eq(time)
    end
  end

  describe 'register_type with MessagePack::Timestamp' do
    let(:factory) do
      factory = MessagePack::Factory.new
      factory.register_type(MessagePack::Timestamp::TYPE, MessagePack::Timestamp)
      factory
    end

    let(:timestamp) { MessagePack::Timestamp.new(Time.now.tv_sec, 123_456_789) }
    it 'serializes and deserializes MessagePack::Timestamp' do
      packed = factory.pack(timestamp)
      unpacked = factory.unpack(packed)
      expect(unpacked).to eq(timestamp)
    end
  end

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
end
