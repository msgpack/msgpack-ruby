require 'msgpack/version'

here = File.expand_path(File.dirname(__FILE__))

require 'rbconfig'
prebuilt = File.join(here, RUBY_PLATFORM, RbConfig::CONFIG['ruby_version'])
if File.directory?(prebuilt)
  require File.join(prebuilt, "msgpack")
else
  require File.join(here, '..', 'ext', 'msgpack', 'msgpack')
end

