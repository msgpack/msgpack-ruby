here = File.expand_path(File.dirname(__FILE__))
require File.join(here, 'msgpack', 'version')
begin
  m = /(\d+.\d+)/.match(RUBY_VERSION)
  ver = m[1]
  ver = '1.9' if ver == '2.0'
  require File.join(here, 'msgpack', ver, 'msgpack')
rescue LoadError
  require File.join(here, 'msgpack', 'msgpack')
end
