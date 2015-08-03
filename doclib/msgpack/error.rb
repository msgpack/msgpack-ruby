module MessagePack

  class UnpackError < StandardError
  end

  class MalformedFormatError < UnpackError
  end

  class StackError < UnpackError
  end

  module TypeError
  end

  class UnexpectedTypeError < UnpackError
    include TypeError
  end
end
