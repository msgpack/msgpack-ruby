# encoding: ascii-8bit
require 'spec_helper'

RSpec.describe MessagePack::ExtensionValue do
  subject do
    described_class.new(1, "data")
  end

  describe '#type/#type=' do
    it 'returns value set by #initialize' do
      expect(subject.type).to eq 1
    end

    it 'assigns a value' do
      subject.type = 2
      expect(subject.type).to eq 2
    end
  end

  describe '#payload/#payload=' do
    it 'returns value set by #initialize' do
      expect(subject.payload).to eq "data"
    end

    it 'assigns a value' do
      subject.payload = "a"
      expect(subject.payload).to eq "a"
    end
  end

  describe '#==/#eql?/#hash' do
    it 'returns equivalent if the content is same' do
      ext1 = MessagePack::ExtensionValue.new(1, "data")
      ext2 = MessagePack::ExtensionValue.new(1, "data")
      expect((ext1 == ext2)).to be true
      expect(ext1.eql?(ext2)).to be true
      expect((ext1.hash == ext2.hash)).to be true
    end

    it 'returns not equivalent if type is not same' do
      ext1 = MessagePack::ExtensionValue.new(1, "data")
      ext2 = MessagePack::ExtensionValue.new(2, "data")
      expect((ext1 == ext2)).to be false
      expect(ext1.eql?(ext2)).to be false
      expect((ext1.hash == ext2.hash)).to be false
    end

    it 'returns not equivalent if payload is not same' do
      ext1 = MessagePack::ExtensionValue.new(1, "data")
      ext2 = MessagePack::ExtensionValue.new(1, "value")
      expect((ext1 == ext2)).to be false
      expect(ext1.eql?(ext2)).to be false
      expect((ext1.hash == ext2.hash)).to be false
    end
  end

  describe '#to_msgpack' do
    it 'serializes very short payload' do
      ext = MessagePack::ExtensionValue.new(1, "a"*2).to_msgpack
      expect(ext).to eq "\xd5\x01" + "a"*2
    end

    it 'serializes short payload' do
      ext = MessagePack::ExtensionValue.new(1, "a"*18).to_msgpack
      expect(ext).to eq "\xc7\x12\x01" + "a"*18
    end

    it 'serializes long payload' do
      ext = MessagePack::ExtensionValue.new(1, "a"*65540).to_msgpack
      expect(ext).to eq "\xc9\x00\x01\x00\x04\x01" + "a"*65540
    end

    it 'with a packer serializes to a packer' do
      ext = MessagePack::ExtensionValue.new(1, "aa")
      packer = MessagePack::Packer.new
      ext.to_msgpack(packer)
      expect(packer.buffer.to_s).to eq "\xd5\x01aa"
    end

    [-129, -65540, -(2**40), 128, 65540, 2**40].each do |type|
      context "with invalid type (#{type})" do
        it 'raises RangeError' do
          expect { MessagePack::ExtensionValue.new(type, "a").to_msgpack }.to raise_error(RangeError)
        end
      end
    end
  end

  describe '#dup' do
    it 'duplicates' do
      ext1 = MessagePack::ExtensionValue.new(1, "data")
      ext2 = ext1.dup
      ext2.type = 2
      ext2.payload = "data2"
      expect(ext1.type).to eq 1
      expect(ext1.payload).to eq "data"
    end
  end
end
