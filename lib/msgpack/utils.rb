if RUBY_PLATFORM =~ /java/

classes = [ Array, Bignum, FalseClass, Fixnum, Float, Hash, NilClass, String, Symbol, TrueClass ]
classes.each { |cl|
  cl.class_eval do
    def to_msgpack(out = '')
      return MessagePack.pack(self, out)
    end
  end
}

end
