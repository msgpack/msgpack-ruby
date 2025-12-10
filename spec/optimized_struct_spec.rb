# frozen_string_literal: true

require 'spec_helper'

RSpec.describe 'optimized_struct' do
  let(:point_struct) { Struct.new(:x, :y) }
  let(:person_struct) { Struct.new(:name, :age, :email) }
  let(:keyword_struct) { Struct.new(:foo, :bar, keyword_init: true) }

  describe 'basic functionality' do
    it 'serializes and deserializes a simple struct' do
      factory = MessagePack::Factory.new
      factory.register_type(0x01, point_struct, optimized_struct: true)

      point = point_struct.new(10, 20)
      packed = factory.dump(point)
      restored = factory.load(packed)

      expect(restored).to be_a(point_struct)
      expect(restored.x).to eq(10)
      expect(restored.y).to eq(20)
    end

    it 'serializes and deserializes a struct with various field types' do
      factory = MessagePack::Factory.new
      factory.register_type(0x01, person_struct, optimized_struct: true)

      person = person_struct.new('Alice', 30, 'alice@example.com')
      packed = factory.dump(person)
      restored = factory.load(packed)

      expect(restored.name).to eq('Alice')
      expect(restored.age).to eq(30)
      expect(restored.email).to eq('alice@example.com')
    end

    it 'handles nil fields' do
      factory = MessagePack::Factory.new
      factory.register_type(0x01, person_struct, optimized_struct: true)

      person = person_struct.new('Bob', nil, nil)
      packed = factory.dump(person)
      restored = factory.load(packed)

      expect(restored.name).to eq('Bob')
      expect(restored.age).to be_nil
      expect(restored.email).to be_nil
    end
  end

  describe 'keyword_init structs' do
    it 'serializes and deserializes keyword_init structs' do
      factory = MessagePack::Factory.new
      factory.register_type(0x01, keyword_struct, optimized_struct: true)

      obj = keyword_struct.new(foo: 'hello', bar: 42)
      packed = factory.dump(obj)
      restored = factory.load(packed)

      expect(restored.foo).to eq('hello')
      expect(restored.bar).to eq(42)
    end
  end

  describe 'nested structs' do
    let(:address_struct) { Struct.new(:street, :city) }
    let(:employee_struct) { Struct.new(:name, :address) }

    it 'handles nested optimized structs' do
      factory = MessagePack::Factory.new
      factory.register_type(0x01, address_struct, optimized_struct: true)
      factory.register_type(0x02, employee_struct, optimized_struct: true)

      address = address_struct.new('123 Main St', 'Springfield')
      employee = employee_struct.new('John', address)

      packed = factory.dump(employee)
      restored = factory.load(packed)

      expect(restored.name).to eq('John')
      expect(restored.address).to be_a(address_struct)
      expect(restored.address.street).to eq('123 Main St')
      expect(restored.address.city).to eq('Springfield')
    end

    it 'handles self-referential structs (same type nested)' do
      person_struct = Struct.new(:name, :friends)

      factory = MessagePack::Factory.new
      factory.register_type(0x01, person_struct, optimized_struct: true)

      alice = person_struct.new('Alice', [])
      bob = person_struct.new('Bob', [])
      charlie = person_struct.new('Charlie', [alice, bob])

      packed = factory.dump(charlie)
      restored = factory.load(packed)

      expect(restored.name).to eq('Charlie')
      expect(restored.friends.length).to eq(2)
      expect(restored.friends[0]).to be_a(person_struct)
      expect(restored.friends[0].name).to eq('Alice')
      expect(restored.friends[1].name).to eq('Bob')
    end
  end

  describe 'arrays and hashes' do
    let(:container_struct) { Struct.new(:items, :metadata) }

    it 'handles structs containing arrays and hashes' do
      factory = MessagePack::Factory.new
      factory.register_type(0x01, container_struct, optimized_struct: true)

      obj = container_struct.new([1, 2, 3], { 'key' => 'value' })
      packed = factory.dump(obj)
      restored = factory.load(packed)

      expect(restored.items).to eq([1, 2, 3])
      expect(restored.metadata).to eq({ 'key' => 'value' })
    end

    it 'handles arrays of structs' do
      factory = MessagePack::Factory.new
      factory.register_type(0x01, point_struct, optimized_struct: true)

      points = [point_struct.new(1, 2), point_struct.new(3, 4), point_struct.new(5, 6)]
      packed = factory.dump(points)
      restored = factory.load(packed)

      expect(restored.length).to eq(3)
      expect(restored[0].x).to eq(1)
      expect(restored[1].x).to eq(3)
      expect(restored[2].x).to eq(5)
    end
  end

  describe 'error handling' do
    it 'raises error when optimized_struct is used with non-Struct class' do
      factory = MessagePack::Factory.new

      expect do
        factory.register_type(0x01, String, optimized_struct: true)
      end.to raise_error(ArgumentError, /optimized_struct.*requires.*Struct/)
    end

    it 'raises error when optimized_struct is used with a module' do
      factory = MessagePack::Factory.new
      some_module = Module.new

      expect do
        factory.register_type(0x01, some_module, optimized_struct: true)
      end.to raise_error(ArgumentError, /optimized_struct.*requires.*Struct/)
    end

    it 'raises error when optimized_struct is used with packer option' do
      factory = MessagePack::Factory.new

      expect do
        factory.register_type(0x01, point_struct,
                              optimized_struct: true,
                              packer: ->(obj) { obj.to_a.pack('l*') })
      end.to raise_error(ArgumentError, /optimized_struct.*cannot be used with packer or unpacker/)
    end

    it 'raises error when optimized_struct is used with unpacker option' do
      factory = MessagePack::Factory.new

      expect do
        factory.register_type(0x01, point_struct,
                              optimized_struct: true,
                              unpacker: ->(data) { point_struct.new(*data.unpack('l*')) })
      end.to raise_error(ArgumentError, /optimized_struct.*cannot be used with packer or unpacker/)
    end

    it 'raises error when optimized_struct is used with recursive option' do
      factory = MessagePack::Factory.new

      expect do
        factory.register_type(0x01, point_struct,
                              optimized_struct: true,
                              recursive: true)
      end.to raise_error(ArgumentError, /optimized_struct.*cannot be used with recursive/)
    end

    it 'propagates exceptions from nested serialization' do
      error_struct = Struct.new(:value)
      bad_object = Object.new
      def bad_object.to_msgpack(_packer)
        raise 'intentional error'
      end

      factory = MessagePack::Factory.new
      factory.register_type(0x01, error_struct, optimized_struct: true)

      obj = error_struct.new(bad_object)
      expect { factory.dump(obj) }.to raise_error(RuntimeError, 'intentional error')
    end
  end

  describe 'performance comparison' do
    let(:large_struct) do
      Struct.new(:f1, :f2, :f3, :f4, :f5, :f6, :f7, :f8, :f9, :f10)
    end

    it 'produces same results as recursive packer/unpacker' do
      # Factory with optimized_struct
      factory_optimized = MessagePack::Factory.new
      factory_optimized.register_type(0x01, large_struct, optimized_struct: true)

      # Factory with traditional recursive packer/unpacker
      factory_recursive = MessagePack::Factory.new
      factory_recursive.register_type(0x01, large_struct,
                                      packer: lambda { |obj, packer|
                                        large_struct.members.each { |m| packer.write(obj.send(m)) }
                                      },
                                      unpacker: lambda { |unpacker|
                                        large_struct.new(*large_struct.members.map { unpacker.read })
                                      },
                                      recursive: true)

      obj = large_struct.new(1, 'two', 3.0, nil, true, false, [1, 2], { 'a' => 1 }, 'nine', 10)

      packed_optimized = factory_optimized.dump(obj)
      packed_recursive = factory_recursive.dump(obj)

      # Wire format should be identical
      expect(packed_optimized).to eq(packed_recursive)

      # Both should round-trip correctly
      restored_optimized = factory_optimized.load(packed_optimized)
      restored_recursive = factory_recursive.load(packed_recursive)

      # Compare field by field (Symbol keys become String keys in msgpack)
      large_struct.members.each do |member|
        expect(restored_optimized.send(member)).to eq(restored_recursive.send(member))
      end
    end
  end

  describe 'edge cases' do
    it 'handles empty structs (0 fields)' do
      empty_struct = Struct.new

      factory = MessagePack::Factory.new
      factory.register_type(0x01, empty_struct, optimized_struct: true)

      obj = empty_struct.new
      packed = factory.dump(obj)
      restored = factory.load(packed)

      expect(restored).to be_a(empty_struct)
    end

    it 'handles structs with many fields' do
      # Create a struct with 100 fields to stress test RB_ALLOCV
      field_names = (1..100).map { |i| :"field_#{i}" }
      many_fields_struct = Struct.new(*field_names)

      factory = MessagePack::Factory.new
      factory.register_type(0x01, many_fields_struct, optimized_struct: true)

      values = (1..100).to_a
      obj = many_fields_struct.new(*values)
      packed = factory.dump(obj)
      restored = factory.load(packed)

      expect(restored).to be_a(many_fields_struct)
      field_names.each_with_index do |field, i|
        expect(restored.send(field)).to eq(i + 1)
      end
    end

    it 'works with Struct subclasses' do
      base_struct = Struct.new(:x, :y)
      subclass = Class.new(base_struct) do
        def magnitude
          Math.sqrt(x**2 + y**2)
        end
      end

      factory = MessagePack::Factory.new
      factory.register_type(0x01, subclass, optimized_struct: true)

      obj = subclass.new(3, 4)
      packed = factory.dump(obj)
      restored = factory.load(packed)

      expect(restored).to be_a(subclass)
      expect(restored.x).to eq(3)
      expect(restored.y).to eq(4)
      expect(restored.magnitude).to eq(5.0)
    end
  end
end
