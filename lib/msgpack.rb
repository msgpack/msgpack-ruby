require "msgpack/version"
begin
  require "msgpack/#{RUBY_VERSION[/\d+.\d+/]}/msgpack"
rescue LoadError
  require "msgpack/msgpack"
end
