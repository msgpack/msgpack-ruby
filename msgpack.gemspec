$LOAD_PATH.push File.expand_path("../lib", __FILE__)
require 'msgpack/version'

Gem::Specification.new do |s|
  s.name = "msgpack"
  s.version = MessagePack::VERSION
  s.summary = "MessagePack, a binary-based efficient data interchange format."
  s.description = %q{MessagePack is a binary-based efficient object serialization library. It enables to exchange structured objects between many languages like JSON. But unlike JSON, it is very fast and small.}
  s.authors = ["Sadayuki Furuhashi", "Theo Hultberg"]
  s.email = ["frsyuki@gmail.com", "theo@iconara.net"]
  s.license = "Apache 2.0"
  s.homepage = "http://msgpack.org/"
  s.rubyforge_project = "msgpack"
  s.has_rdoc = false
  s.require_paths = ["lib"]
  if /java/ =~ RUBY_PLATFORM
    s.files = Dir['lib/**/*.rb', 'lib/**/*.jar']
    s.platform = Gem::Platform.new('java')
  else
    s.files = `git ls-files`.split("\n")
    s.extensions = ["ext/msgpack/extconf.rb"]
  end
  s.test_files = `git ls-files -- {test,spec}/*`.split("\n")

  s.add_development_dependency 'bundler', ['~> 1.0']
  s.add_development_dependency 'rake', ['~> 0.9.2']
  s.add_development_dependency 'rake-compiler', ['~> 0.8.3']
  s.add_development_dependency 'rspec', ['~> 3.3']
  s.add_development_dependency 'yard', ['~> 0.8.2']
  s.add_development_dependency 'json'
  s.add_development_dependency 'activesupport'
end
