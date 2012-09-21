module MessagePack

  class UnpackError < StandardError
  end

  class MalformedFormatError < UnpackError
  end

  class StackError < UnpackError
  end

  class TypeError < StandardError
  end
end
