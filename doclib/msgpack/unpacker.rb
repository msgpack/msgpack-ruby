module MessagePack

  #
  # MessagePack::Unpacker is an interface to deserialize objects from an internal buffer,
  # which is a MessagePack::Buffer.
  #
  class Unpacker
    #
    # Creates a MessagePack::Unpacker instance.
    # Currently, no options are supported.
    #
    # @overload initialize(options={})
    #   @param options [Hash]
    #
    # @overload initialize(io, options={})
    #   @param io [IO]
    #   @param options [Hash]
    #   This unpacker reads data from the _io_ to fill the internal buffer.
    #   _io_ must respond to readpartial(length,string) or read(length,string) method.
    #
    def initialize(*args)
    end

    #
    # Internal buffer
    #
    # @return MessagePack::Unpacker
    #
    attr_reader :buffer

    #
    # Deserializes an object from internal buffer and returns it.
    #
    # @return [Object] deserialized object
    #
    def read
    end

    alias unpack read

    #
    # Deserializes an object and ignores it. This method is faster than _read_.
    #
    # @return nil
    #
    def skip
    end

    #
    # Deserializes a nil value if it exists and returns _true_.
    # Otherwise, if a byte exists but the byte is not nil value or the internal buffer is empty, returns _false_.
    #
    # @return [Boolean]
    #
    def skip
    end

    #
    # Read a header of an array and returns its size.
    # It converts a serialized array into a stream of elements.
    #
    # If the serialized object is not an array, it raises MessagePack::TypeError.
    #
    # @return [Integer] size of the array
    #
    def read_array_header
    end

    #
    # Read a header of an map and returns its size.
    # It converts a serialized map into a stream of key-value pairs.
    #
    # If the serialized object is not a map, it raises MessagePack::TypeError.
    #
    # @return [Integer] size of the map
    #
    def read_map_header
    end

    #
    # Appends data into the internal buffer.
    # This method calls buffer.append(data).
    #
    # @param data [String]
    #
    def feed(data)
    end

    #
    # Repeats to deserialize objects.
    #
    # It repeats until the internal buffer does not include any complete objects.
    #
    # If the internal IO was set, it reads data from the IO when the buffer becomes empty,
    # and returns when the IO raised EOFError.
    #
    # @yieldparam object [Object] deserialized object
    # @return nil
    #
    def each(&block)
    end

    #
    # Appends data into the internal buffer and repeats to deserialize objects.
    # This method is equals to feed(data) && each.
    #
    # @param data [String]
    # @yieldparam object [Object] deserialized object
    # @return nil
    #
    def feed_each(data, &block)
    end
  end

end
