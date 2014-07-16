require "msgpack/version"

if defined?(RUBY_ENGINE) && RUBY_ENGINE == 'jruby'
  require 'jruby'
  require 'msgpack/msgpack.jar'
  org.msgpack.jruby.MessagePackLibrary.new.load(JRuby.runtime, false)
else
  begin
    require "msgpack/#{RUBY_VERSION[/\d+.\d+/]}/msgpack"
  rescue LoadError
    require "msgpack/msgpack"
  end
end
