module MessagePack
  class Unpacker
    # see ext for other methods

    def type_registered?(klass_or_type)
      types = registered_types # type -> proc

      case klass_or_type
      when Class
        types.each_pair do |type, uk_proc|
          if uk_proc.is_a?(Method) && uk_proc.receiver == klass_or_type
            return true
          end
        end
        false
      when Integer
        types.has_key?(klass_or_type)
      else
        raise ArgumentError, "class or type id"
      end
    end
  end
end
