
if ENV['SIMPLE_COV']
  require 'simplecov'
  SimpleCov.start do
    add_filter 'spec/'
    add_filter 'pkg/'
    add_filter 'vendor/'
  end
end

if ENV['GC_STRESS']
  puts "enable GC.stress"
  GC.stress = true
end

require 'msgpack'

Packer = MessagePack::Packer
Unpacker = MessagePack::Unpacker
Buffer = MessagePack::Buffer

