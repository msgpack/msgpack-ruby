require 'msgpack/version'

module MessagePack
  if RUBY_PLATFORM =~ /java/
    begin
      require 'java'

      # load dependencies of MessagePack for Java
      require 'slf4j-api-1.6.1.jar'
      require 'slf4j-log4j12-1.6.1.jar'
      require 'log4j-1.2.16.jar'
      require 'json-simple-1.1.jar'
      require 'javassist-3.15.0-GA.jar'
      # load MessagePack for Java
      require 'msgpack-0.6.0-devel.jar'

      # load Java extension of MessagePack for Ruby
      require 'msgpack/msgpack.jar'

      # add to_msgpack method in following classes
      classes = [ Array, Bignum, FalseClass, Fixnum, Float, Hash, NilClass, String, Symbol, TrueClass ]
      classes.each { |cl|
        cl.class_eval do
          def to_msgpack(out = '')
            return MessagePack.pack(self, out)
          end
        end
      }
    rescue LoadError => e
      raise e
    end
  else
    # load C extension of MessagePack for Ruby
    require 'msgpack.so'
  end
end
