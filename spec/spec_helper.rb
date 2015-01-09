
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

if /java/ =~ RUBY_PLATFORM
  RSpec.configure do |c|
    c.treat_symbols_as_metadata_keys_with_true_values = true
    c.filter_run_excluding :encodings => !(defined? Encoding)
  end
else
  Packer = MessagePack::Packer
  Unpacker = MessagePack::Unpacker
  Buffer = MessagePack::Buffer
end
