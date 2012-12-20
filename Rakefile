require 'fileutils'

task :update do
  if File.directory?("msgpack-ruby")
    Dir.glob("msgpack-ruby/*", &FileUtils.method(:rm_rf))
    Dir.chdir("msgpack-ruby") do
      sh "git checkout ."
    end
  else
    sh "git clone git://github.com/msgpack/msgpack-ruby.git"
  end

  Dir.chdir("msgpack-ruby") do
    sh "git checkout master"
    sh "git pull"
    sh "rake doc"
  end

  FileUtils.cp_r Dir.glob("msgpack-ruby/doc/*"), '.'
end

task :default => :update

