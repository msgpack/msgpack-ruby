module MessagePack

  #
  # MessagePack::Unpacker is an interface to deserialize objects from an internal buffer,
  # which is a MessagePack::Buffer.
  #
  class Unpacker
    #
    # Creates a MessagePack::Unpacker instance.
    #
    # @overload initialize(options={})
    #   @param options [Hash]
    #
    # @overload initialize(io, options={})
    #   @param io [IO]
    #   @param options [Hash]
    #   This unpacker reads data from the _io_ to fill the internal buffer.
    #   _io_ must respond to readpartial(length [,string]) or read(length [,string]) method.
    #
    # Supported options:
    #
    # * *:symbolize_keys* deserialize keys of Hash objects as Symbol instead of String
    # * *:encoding* set the default encoding for unpacked Strings
    #
    # See also Buffer#initialize for other options.
    #
    def initialize(*args)
    end

    #
    # Internal buffer
    #
    # @return [MessagePack::Buffer]
    #
    attr_reader :buffer

    #
    # Deserializes an object from internal buffer and returns it.
    #
    # If there're not enough buffer, this method raises EOFError.
    # If data format is invalid, this method raises MessagePack::MalformedFormatError.
    # If the stack is too deep, this method raises MessagePack::StackError.
    #
    # @return [Object] deserialized object
    #
    def read
    end

    alias unpack read

    #
    # Deserializes an object and ignores it. This method is faster than _read_.
    #
    # This method could raise same errors with _read_.
    #
    # @return nil
    #
    def skip
    end

    #
    # Deserializes a nil value if it exists and returns _true_.
    # Otherwise, if a byte exists but the byte doesn't represent nil value,
    # returns _false_.
    #
    # If there're not enough buffer, this method raises EOFError.
    #
    # @return [Boolean]
    #
    def skip_nil
    end

    #
    # Read a header of an array and returns its size.
    # It converts a serialized array into a stream of elements.
    #
    # If the serialized object is not an array, it raises MessagePack::TypeError.
    # If there're not enough buffer, this method raises EOFError.
    #
    # @return [Integer] size of the array
    #
    def read_array_header
    end

    #
    # Reads a header of an map and returns its size.
    # It converts a serialized map into a stream of key-value pairs.
    #
    # If the serialized object is not a map, it raises MessagePack::TypeError.
    # If there're not enough buffer, this method raises EOFError.
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
    # @return [Unpacker] self
    #
    def feed(data)
    end

    #
    # Repeats to deserialize objects.
    #
    # It repeats until the internal buffer does not include any complete objects.
    #
    # If the an IO is set, it repeats to read data from the IO when the buffer
    # becomes empty until the IO raises EOFError.
    #
    # This method could raise same errors with _read_ excepting EOFError.
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

    #
    # Resets deserialization state of the unpacker and clears the internal buffer.
    #
    # @return nil
    #
    def reset
    end
  end

end
