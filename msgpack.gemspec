$:.push File.expand_path("../lib", __FILE__)
require 'msgpack/version'

Gem::Specification.new do |s|
  s.name = "msgpack"
  s.version = MessagePack::VERSION
  s.summary = "MessagePack, a binary-based efficient data interchange format."
  s.description = %q{MessagePack is a binary-based efficient object serialization library. It enables to exchange structured objects between many languages like JSON. But unlike JSON, it is very fast and small.}
  s.author = "FURUHASHI Sadayuki"
  s.email = "frsyuki@gmail.com"
  s.homepage = "http://msgpack.org/"
  s.rubyforge_project = "msgpack"
  s.has_rdoc = false
  s.files = `git ls-files`.split("\n")
  s.test_files = `git ls-files -- {test,spec}/*`.split("\n")
  s.require_paths = ["lib"]
  s.extensions = ["ext/msgpack/extconf.rb"]

  s.add_development_dependency 'bundler', ['~> 1.0']
  s.add_development_dependency 'rake', ['~> 0.9.2']
  s.add_development_dependency 'rake-compiler', ['~> 0.8.3']
  s.add_development_dependency 'rspec', ['~> 2.11']
  s.add_development_dependency 'json', ['~> 1.7']
  s.add_development_dependency 'yard', ['~> 0.8.2']
end
