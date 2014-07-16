require "msgpack/version"
if RUBY_PLATFORM =~ /java/
  require 'java'
  require 'msgpack/java/javassist-3.15.0-GA'
  require 'msgpack/java/msgpack-0.6.6'
end
begin
  require "msgpack/#{RUBY_VERSION[/\d+.\d+/]}/msgpack"
rescue LoadError
  require "msgpack/msgpack"
end
