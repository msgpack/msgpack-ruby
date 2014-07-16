require "msgpack/version"
if RUBY_PLATFORM =~ /java/
  require 'java'
  require 'msgpack/javassist-3.15.0-GA'
  require 'msgpack/msgpack-0.6.6'
  require 'msgpack/msgpack'
else
  begin
    require "msgpack/#{RUBY_VERSION[/\d+.\d+/]}/msgpack"
  rescue LoadError
    require "msgpack/msgpack"
  end
end
