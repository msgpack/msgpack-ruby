# % bundle install
# % bundle exec ruby bench/bench.rb

require 'msgpack'

require 'benchmark/ips'

object_plain = {
  'message' => '127.0.0.1 - - [10/Oct/2000:13:55:36 -0700] "GET /apache_pb.gif HTTP/1.0" 200 2326 "http://www.example.com/start.html" "Mozilla/4.08 [en] (Win98; I ;Nav)"'
}

data_plain = MessagePack.pack(object_plain)

object_structured = {
  'remote_host' => '127.0.0.1',
  'remote_user' => '-',
  'date' => '10/Oct/2000:13:55:36 -0700',
  'request' => 'GET /apache_pb.gif HTTP/1.0',
  'method' => 'GET',
  'path' => '/apache_pb.gif',
  'protocol' => 'HTTP/1.0',
  'status' => 200,
  'bytes' => 2326,
  'referer' => 'http://www.example.com/start.html',
  'agent' => 'Mozilla/4.08 [en] (Win98; I ;Nav)'
}

data_structured = MessagePack.pack(object_structured)

class Extended
  def to_msgpack_ext
    MessagePack.pack({})
  end

  def self.from_msgpack_ext(data)
    MessagePack.unpack(data)
    Extended.new
  end
end

object_extended = {
  'extended' => Extended.new
}

extended_packer = MessagePack::Packer.new
extended_packer.register_type(0x00, Extended, :to_msgpack_ext)
data_extended = extended_packer.pack(object_extended).to_s

# Struct for optimized_struct benchmarks
BenchStruct = Struct.new(:name, :age, :email, :score, :active)

# Factory with optimized_struct (C-level fast path)
factory_optimized = MessagePack::Factory.new
factory_optimized.register_type(0x01, BenchStruct, optimized_struct: true)

# Factory with recursive packer/unpacker (Ruby-level)
factory_recursive = MessagePack::Factory.new
factory_recursive.register_type(
  0x01,
  BenchStruct,
  packer: lambda { |obj, packer|
    packer.write(obj.name)
    packer.write(obj.age)
    packer.write(obj.email)
    packer.write(obj.score)
    packer.write(obj.active)
  },
  unpacker: lambda { |unpacker|
    BenchStruct.new(unpacker.read, unpacker.read, unpacker.read, unpacker.read, unpacker.read)
  },
  recursive: true
)

object_struct = BenchStruct.new('Alice', 30, 'alice@example.com', 95.5, true)
data_struct_optimized = factory_optimized.dump(object_struct)
data_struct_recursive = factory_recursive.dump(object_struct)

# Pre-create packers/unpackers for fair comparison (avoid factory overhead in loop)
packer_optimized = factory_optimized.packer
packer_recursive = factory_recursive.packer
unpacker_optimized = factory_optimized.unpacker
unpacker_recursive = factory_recursive.unpacker

Benchmark.ips do |x|
  x.report('pack-plain') do
    MessagePack.pack(object_plain)
  end

  x.report('pack-structured') do
    MessagePack.pack(object_structured)
  end

  x.report('pack-extended') do
    packer = MessagePack::Packer.new
    packer.register_type(0x00, Extended, :to_msgpack_ext)
    packer.pack(object_extended).to_s
  end

  x.report('pack-struct-optimized') do
    packer_optimized.write(object_struct)
    packer_optimized.to_s
    packer_optimized.clear
  end

  x.report('pack-struct-recursive') do
    packer_recursive.write(object_struct)
    packer_recursive.to_s
    packer_recursive.clear
  end

  x.compare!
end

Benchmark.ips do |x|
  x.report('unpack-plain') do
    MessagePack.unpack(data_plain)
  end

  x.report('unpack-structured') do
    MessagePack.unpack(data_structured)
  end

  x.report('unpack-extended') do
    unpacker = MessagePack::Unpacker.new
    unpacker.register_type(0x00, Extended, :from_msgpack_ext)
    unpacker.feed data_extended
    unpacker.read
  end

  x.report('unpack-struct-optimized') do
    unpacker_optimized.feed(data_struct_optimized)
    unpacker_optimized.read
    unpacker_optimized.reset
  end

  x.report('unpack-struct-recursive') do
    unpacker_recursive.feed(data_struct_recursive)
    unpacker_recursive.read
    unpacker_recursive.reset
  end

  x.compare!
end
