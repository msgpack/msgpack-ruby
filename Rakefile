
require 'bundler'
Bundler::GemHelper.install_tasks

require 'fileutils'

require 'rspec/core'
require 'rspec/core/rake_task'
require 'yard'

RSpec::Core::RakeTask.new(:spec) do |t|
  t.rspec_opts = ["-c", "-f progress"]
  t.rspec_opts << "-Ilib"
  t.pattern = 'spec/**/*_spec.rb'
  t.verbose = true
end

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
    #jruby_home = RbConfig::CONFIG['prefix']
    #jars = ["#{jruby_home}/lib/jruby.jar"] + FileList['lib/*.jar']
    #ext.classpath = jars.map { |x| File.expand_path x }.join ':'
  end

else
  require 'rake/extensiontask'

  Rake::ExtensionTask.new('msgpack', spec) do |ext|
    ext.cross_compile = true
    ext.lib_dir = File.join(*['lib', 'msgpack', ENV['FAT_DIR']].compact)
    #ext.cross_platform = 'i386-mswin32'
  end
end

task :default => :build


###
## Cross compile memo
##
## Ubuntu Ubuntu 10.04.1 LTS
##
#
### install mingw32 cross compiler
# sudo apt-get install gcc-mingw32
#
### install rbenv
# git clone git://github.com/sstephenson/rbenv.git ~/.rbenv
# echo 'export PATH="$HOME/.rbenv/bin:$PATH"' >> ~/.bash_profile
# echo 'eval "$(rbenv init -)"' >> ~/.bash_profile
# exec $SHELL -l
#
### install cross-compiled ruby 2.0.0
# rbenv install 2.0.0-p0
# gem install rake-compiler
# rake-compiler cross-ruby VERSION=2.0.0-p0
#
### install cross-compiled ruby 1.9.3
# rbenv install 1.9.3-p327
# gem install rake-compiler
# rake-compiler cross-ruby VERSION=1.9.3-p327
#
### install cross-compiled ruby 1.8.7
# rbenv install 1.8.7-p371
# gem install rake-compiler
# rake-compiler cross-ruby VERSION=1.8.7-p371
#
### build gem
# rbenv shell 1.8.7-p371
# gem install bundler && bundle
# rake cross native gem RUBY_CC_VERSION=1.8.7:1.9.3:2.0.0
#

