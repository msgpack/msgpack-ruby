
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

def java?
  /java/ =~ RUBY_PLATFORM
end

if java?
  RSpec.configure do |c|
    c.filter_run_excluding encodings: !(defined? Encoding)
    c.expose_dsl_globally = false
  end
else
  RSpec.configure do |config|
    config.expect_with :rspec do |c|
      c.syntax = [:should, :expect]
    end
    config.expose_dsl_globally = false
  end
  Packer = MessagePack::Packer
  Unpacker = MessagePack::Unpacker
  Buffer = MessagePack::Buffer
  Factory = MessagePack::Factory
  ExtensionValue = MessagePack::ExtensionValue
end
