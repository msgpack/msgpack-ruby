class Symbol
  def to_msgpack_ext
    [to_s].pack('A*')
  end

  def self.from_msgpack_ext(data)
    data.unpack1('A*').to_sym
  end
end
