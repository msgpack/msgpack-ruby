class MessagePack::Extended
  attr_reader :type, :data

  def initialize(type, data)
    @type = type
    @data = data
  end

  def to_msgpack(pk = nil)
    return MessagePack.pack(self, pk) unless pk.class == MessagePack::Packer
    pk.write_ext_header(@data.bytesize, @type)
    pk.buffer.write(@data)
    return pk
  end

  def ==(other)
    type == other.type && data == other.data
  end
end
