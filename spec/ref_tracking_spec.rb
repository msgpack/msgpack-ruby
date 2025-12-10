# frozen_string_literal: true

require 'spec_helper'

RSpec.describe 'ref_tracking option' do
  # Define a test struct
  TestStruct = Struct.new(:name, :value, :nested)

  let(:factory) do
    factory = MessagePack::Factory.new
    factory.register_type(
      0x01,
      TestStruct,
      packer: lambda { |obj, pk|
        pk.write(obj.name)
        pk.write(obj.value)
        pk.write(obj.nested)
      },
      unpacker: lambda { |uk|
        TestStruct.new(uk.read, uk.read, uk.read)
      },
      recursive: true,
      ref_tracking: true
    )
    factory
  end

  describe 'packing with ref_tracking' do
    it 'writes new-ref marker for first occurrence' do
      obj = TestStruct.new('test', 42, nil)
      packed = factory.packer.write(obj).to_s

      # Should be able to unpack it
      unpacked = factory.unpacker.feed(packed).read
      expect(unpacked).to be_a(TestStruct)
      expect(unpacked.name).to eq('test')
      expect(unpacked.value).to eq(42)
    end

    it 'writes back-reference for repeated objects' do
      shared = TestStruct.new('shared', 100, nil)
      container = TestStruct.new('parent', 0, shared)

      # Pack the shared object twice in an array
      packed = factory.packer.write([shared, shared, container]).to_s

      # Unpack and verify
      unpacked = factory.unpacker.feed(packed).read
      expect(unpacked).to be_an(Array)
      expect(unpacked.length).to eq(3)

      # All three references should resolve to objects with the same values
      expect(unpacked[0].name).to eq('shared')
      expect(unpacked[1].name).to eq('shared')
      expect(unpacked[2].nested.name).to eq('shared')
    end

    it 'handles deeply nested structures with shared references' do
      leaf = TestStruct.new('leaf', 1, nil)
      mid1 = TestStruct.new('mid1', 2, leaf)
      mid2 = TestStruct.new('mid2', 3, leaf) # shares leaf
      root = TestStruct.new('root', 0, [mid1, mid2])

      packed = factory.packer.write(root).to_s
      unpacked = factory.unpacker.feed(packed).read

      expect(unpacked.name).to eq('root')
      expect(unpacked.nested[0].name).to eq('mid1')
      expect(unpacked.nested[1].name).to eq('mid2')
      expect(unpacked.nested[0].nested.name).to eq('leaf')
      expect(unpacked.nested[1].nested.name).to eq('leaf')
    end
  end

  describe 'deduplication effectiveness' do
    it 'produces smaller output when objects are repeated' do
      shared = TestStruct.new('a' * 100, 42, nil)

      # Pack array with same object repeated many times
      repeated = [shared] * 10
      packed_with_ref_tracking = factory.packer.write(repeated).to_s

      # Pack without ref_tracking
      factory_without_ref = MessagePack::Factory.new
      factory_without_ref.register_type(
        0x01,
        TestStruct,
        packer: lambda { |obj, pk|
          pk.write(obj.name)
          pk.write(obj.value)
          pk.write(obj.nested)
        },
        unpacker: lambda { |uk|
          TestStruct.new(uk.read, uk.read, uk.read)
        },
        recursive: true
      )
      packed_without_ref_tracking = factory_without_ref.packer.write(repeated).to_s

      # With ref_tracking should be smaller since object is only serialized once
      expect(packed_with_ref_tracking.bytesize).to be < packed_without_ref_tracking.bytesize
    end
  end

  describe 'combined with optimized_struct' do
    OptimizedStruct = Struct.new(:x, :y, :ref)

    let(:optimized_factory) do
      factory = MessagePack::Factory.new
      factory.register_type(
        0x02,
        OptimizedStruct,
        optimized_struct: true,
        ref_tracking: true
      )
      factory
    end

    it 'works with both optimized_struct and ref_tracking' do
      shared = OptimizedStruct.new(1, 2, nil)
      parent = OptimizedStruct.new(10, 20, shared)

      packed = optimized_factory.packer.write([shared, parent]).to_s
      unpacked = optimized_factory.unpacker.feed(packed).read

      expect(unpacked[0].x).to eq(1)
      expect(unpacked[0].y).to eq(2)
      expect(unpacked[1].x).to eq(10)
      expect(unpacked[1].y).to eq(20)
      expect(unpacked[1].ref.x).to eq(1)
    end

    it 'deduplicates repeated optimized structs' do
      shared = OptimizedStruct.new(1, 2, nil)

      packed = optimized_factory.packer.write([shared, shared, shared]).to_s
      unpacked = optimized_factory.unpacker.feed(packed).read

      expect(unpacked.length).to eq(3)
      expect(unpacked[0].x).to eq(1)
      expect(unpacked[1].x).to eq(1)
      expect(unpacked[2].x).to eq(1)
    end
  end

  describe 'edge cases' do
    it 'handles nil nested values' do
      obj = TestStruct.new('test', nil, nil)
      packed = factory.packer.write(obj).to_s
      unpacked = factory.unpacker.feed(packed).read

      expect(unpacked.name).to eq('test')
      expect(unpacked.value).to be_nil
      expect(unpacked.nested).to be_nil
    end

    it 'handles empty arrays' do
      packed = factory.packer.write([]).to_s
      unpacked = factory.unpacker.feed(packed).read
      expect(unpacked).to eq([])
    end

    it 'handles single object' do
      obj = TestStruct.new('solo', 1, nil)
      packed = factory.packer.write(obj).to_s
      unpacked = factory.unpacker.feed(packed).read

      expect(unpacked.name).to eq('solo')
    end

    it 'resets ref tracking between pack operations' do
      packer = factory.packer

      obj = TestStruct.new('test', 1, nil)
      packed1 = packer.write(obj).to_s
      packer.clear

      # Pack again - should not reference previous pack's objects
      packed2 = packer.write(obj).to_s

      # Both should unpack successfully
      unpacker = factory.unpacker
      unpacked1 = unpacker.feed(packed1).read
      unpacker.reset
      unpacked2 = unpacker.feed(packed2).read

      expect(unpacked1.name).to eq('test')
      expect(unpacked2.name).to eq('test')
    end
  end

  describe 'ref_id ranges' do
    it 'handles many unique objects (testing ref_id encoding)' do
      # Create many unique objects to test ref_id encoding beyond fixint range
      objects = 150.times.map { |i| TestStruct.new("obj#{i}", i, nil) }

      # Create array that references each twice
      array = objects + objects

      packed = factory.packer.write(array).to_s
      unpacked = factory.unpacker.feed(packed).read

      expect(unpacked.length).to eq(300)
      expect(unpacked[0].name).to eq('obj0')
      expect(unpacked[149].name).to eq('obj149')
      expect(unpacked[150].name).to eq('obj0') # Back-reference
      expect(unpacked[299].name).to eq('obj149') # Back-reference
    end
  end
end
