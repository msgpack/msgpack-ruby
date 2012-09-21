
module MessagePack
  #
  # Serializes an object into an IO or String.
  #
  # @overload dump(obj)
  #   @return [String] serialized data
  #
  # @overload dump(obj, io)
  #   @return [IO]
  #
  def self.dump(arg)
  end

  #
  # Serializes an object into an IO or String. Alias of dump.
  #
  # @overload dump(obj)
  #   @return [String] serialized data
  #
  # @overload dump(obj, io)
  #   @return [IO]
  #
  def self.pack(arg)
  end

  #
  # Deserializes an object from an IO or String.
  #
  # @overload load(string)
  #   @param string [String] data to deserialize
  #
  # @overload load(io)
  #   @param io [IO]
  #
  # @return [Object] deserialized object
  #
  def self.load(arg)
  end

  #
  # Deserializes an object from an IO or String. Alias of load.
  #
  # @overload unpack(string)
  #   @param string [String] data to deserialize
  #
  # @overload unpack(io)
  #   @param io [IO]
  #
  # @return [Object] deserialized object
  #
  def self.unpack(arg)
  end
end

