require 'msgpack/version'

here = File.expand_path(File.dirname(__FILE__))

if RUBY_PLATFORM =~ /java/
  require 'java'
  require File.join(here, 'json-simple-1.1.jar')
  require File.join(here, 'javassist-3.15.0-GA.jar')
  require File.join(here, 'msgpack-0.6.5.jar')
  require File.join(here, 'msgpack/msgpack')
else
  require 'rbconfig'
  prebuilt = File.join(here, RUBY_PLATFORM, RbConfig::CONFIG['ruby_version'])
  if File.directory?(prebuilt)
    require File.join(prebuilt, "msgpack")
  else
    require File.join(here, '..', 'ext', 'msgpack', 'msgpack')
  end
end

