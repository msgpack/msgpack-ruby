# -*- mode: ruby; coding: utf-8 -*-
require 'rubygems'
require 'bundler'

require 'rake'
require 'rake/clean'
require 'rake/testtask'
require 'rake/rdoctask'
require 'rake/gempackagetask'

def jruby?
  !! (RUBY_PLATFORM =~ /java/)
end

if jruby?
  module Bundler
    class GemHelper
      def build_gem
        file_name = nil
        sh("jruby -S gem build '#{spec_path}'") { |out, code|
          raise out unless out[/Successfully/]
          file_name = File.basename(built_gem_path)
          FileUtils.mkdir_p(File.join(base, 'pkg'))
          FileUtils.mv(built_gem_path, 'pkg')
          Bundler.ui.confirm "#{name} #{version} built to pkg/#{file_name}"
        }
        File.join(base, 'pkg', file_name)
      end
    end
  end
end

Bundler::GemHelper.install_tasks

begin
  if jruby?
    require 'rake/javaextensiontask'
  else
    require 'rake/extensiontask'
  end
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

if jruby?
  Rake::JavaExtensionTask.new("msgpack") do |ext|
    jruby_home = RbConfig::CONFIG['prefix']
    ext.ext_dir = 'ext/java'
    ext.lib_dir = 'lib/msgpack'
    jars = ["#{jruby_home}/lib/jruby.jar"] + FileList['lib/*.jar']
    ext.classpath = jars.map { |x| File.expand_path x }.join ':'
  end

else
  Rake::ExtensionTask.new("msgpack") do |ext|
  end
end

