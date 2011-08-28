require 'msgpack/version'

module MessagePack
  if RUBY_PLATFORM =~ /java/
    begin
      require 'java'

      require 'slf4j-api-1.6.1.jar'
      require 'slf4j-log4j12-1.6.1.jar'
      require 'log4j-1.2.16.jar'
      require 'json-simple-1.1.jar'
      require 'javassist-3.15.0-GA.jar'
      require 'msgpack-0.6.0-devel.jar'

      require 'msgpack/msgpack.jar'
      require 'msgpack/msgpack'
      require 'msgpack/utils'
    rescue LoadError => e
      raise e
    end
  else
    require 'msgpack/msgpack.so'
  end
end
