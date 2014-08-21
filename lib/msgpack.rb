require "msgpack/version"
require "msgpack/active_support/time_with_zone"

begin
  require "msgpack/#{RUBY_VERSION[/\d+.\d+/]}/msgpack"
rescue LoadError
  require "msgpack/msgpack"
end
