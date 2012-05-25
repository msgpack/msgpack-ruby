$LOAD_PATH.unshift(File.expand_path('../lib', File.dirname(__FILE__)))

if ENV['SIMPLE_COV']
  require 'simplecov'
  SimpleCov.start do
    add_filter 'spec/'
    add_filter 'pkg/'
    add_filter 'vendor/'
  end
end

require 'msgpack'

include MessagePack

