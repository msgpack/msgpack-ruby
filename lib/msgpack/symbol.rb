class Symbol
  # to_msgpack_ext is supposed to return a binary string.
  # The canonical way to do it for symbols would be:
  #  [to_s].pack('A*')
  # However in this instance we can take a shortcut
  if method_defined?(:name)
    alias_method :to_msgpack_ext, :name
  else
    alias_method :to_msgpack_ext, :to_s
  end

  def self.from_msgpack_ext(data)
    # from_msgpack_ext is supposed to parse a binary string.
    # The canonical way to do it for symbols would be:
    #  data.unpack1('A*').to_sym
    # However in this instance we can take a shortcut
    data.to_sym
  end
end
