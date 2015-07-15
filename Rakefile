
require 'bundler'
Bundler::GemHelper.install_tasks

require 'fileutils'

require 'rspec/core'
require 'rspec/core/rake_task'
require 'yard'

task :spec => :compile

desc 'Run RSpec code examples and measure coverage'
task :coverage do |t|
  ENV['SIMPLE_COV'] = '1'
  Rake::Task["spec"].invoke
end

desc 'Generate YARD document'
YARD::Rake::YardocTask.new(:doc) do |t|
  t.files   = ['lib/msgpack/version.rb','doclib/**/*.rb']
  t.options = []
  t.options << '--debug' << '--verbose' if $trace
end

spec = eval File.read("msgpack.gemspec")

if RUBY_PLATFORM =~ /java/
  require 'rake/javaextensiontask'

  Rake::JavaExtensionTask.new('msgpack', spec) do |ext|
    ext.ext_dir = 'ext/java'
    jruby_home = RbConfig::CONFIG['prefix']
    jars = ["#{jruby_home}/lib/jruby.jar"]
    ext.classpath = jars.map { |x| File.expand_path(x) }.join(':')
    ext.lib_dir = File.join(*['lib', 'msgpack', ENV['FAT_DIR']].compact)
    ext.source_version = '1.6'
    ext.target_version = '1.6'
  end

  RSpec::Core::RakeTask.new(:spec) do |t|
    t.rspec_opts = ["-c", "-f progress"]
    t.rspec_opts << "-Ilib"
    t.pattern = 'spec/{,jruby/}*_spec.rb'
    t.verbose = true
  end

else
  require 'rake/extensiontask'

  Rake::ExtensionTask.new('msgpack', spec) do |ext|
    ext.ext_dir = 'ext/msgpack'
    ext.cross_compile = true
    ext.lib_dir = File.join(*['lib', 'msgpack', ENV['FAT_DIR']].compact)
    ext.cross_platform = ['x86-mingw32', 'x64-mingw32']
  end

  RSpec::Core::RakeTask.new(:spec) do |t|
    t.rspec_opts = ["-c", "-f progress"]
    t.rspec_opts << "-Ilib"
    t.pattern = 'spec/{,cruby/}*_spec.rb'
    t.verbose = true
  end
end

CLEAN.include('lib/msgpack/msgpack.*')

task :default => [:spec, :build, :doc]
