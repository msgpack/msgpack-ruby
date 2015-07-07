module MessagePack
  class Packer
    # see ext for other methods

    def type_registered?(klass_or_type)
      types = registered_types # class -> [type, proc]

      case klass_or_type
      when Class
        types.has_key?(klass_or_type) || types.keys.any?{|klass| klass_or_type <= klass }
      when Integer
        types.each_pair do |klass, ary|
          if ary[0] == klass_or_type
            return true
          end
        end
        false
      else
        raise ArgumentError, "class or type id"
      end
    end
  end
end
