here = File.expand_path(File.dirname(__FILE__))
require File.join(here, 'msgpack', 'version')
begin
  m = /(\d+.\d+)/.match(RUBY_VERSION)
  ver = m[1]
  require File.join(here, 'msgpack', ver, 'msgpack')
rescue LoadError
  require File.join(here, 'msgpack', 'msgpack')
end
