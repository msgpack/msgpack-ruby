module MessagePack
  class Factory
    # see ext for other methods

    def registered_types
      packer, unpacker = registered_types_internal
      # packer: Class -> [id, proc]
      # unpacker: id -> proc

      mapping = {} # id => [Class, packer_proc, unpacker_proc]
      packer.each_pair do |klass, pk|
        mapping[pk[0]] = [klass, pk[1], unpacker[pk[0]]]
      end

      mapping
    end

    def type_registered?(klass_or_type)
      packer, unpacker = registered_types_internal

      case klass_or_type
      when Integer
        unpacker.has_key?(klass_or_type)
      when Class
        packer.has_key?(klass_or_type) || packer.keys.any?{|klass| klass_or_type <= klass }
      else
        raise ArgumentError, "class or type id"
      end
    end
  end
end
