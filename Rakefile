# -*- mode: ruby; coding: utf-8 -*-
require 'rubygems'
require 'bundler'

require 'rake'
require 'rake/clean'
require 'rake/testtask'
require 'rake/rdoctask'
require 'rake/gempackagetask'

Bundler::GemHelper.install_tasks

begin
  require 'rake/extensiontask'
rescue LoadError
  abort "This Rakefile requires rake-compiler (gem install rake-compiler)"
end

desc 'Default: run unit tests.'
task :default => :test

Rake::TestTask.new(:test) do |t|
  t.libs << 'lib'
  t.libs << 'test'
  t.pattern = 'test/**/test_*.rb'
  t.verbose = true
end

desc 'Generate documentation for the msgpack-ruby.'
Rake::RDocTask.new(:rdoc) do |rdoc|
  rdoc.rdoc_dir = 'rdoc'
  rdoc.title = 'MsgPack-Ruby'
  rdoc.options << '--line-numbers' << '--inline-source'
  rdoc.rdoc_files.include('README')
  rdoc.rdoc_files.include('lib/**/*.rb')
  rdoc.rdoc_files.include('ext/**/*.c')
end

Rake::ExtensionTask.new("msgpack") do |ext|
end

