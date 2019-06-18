# frozen_string_literal: true

module MessagePack
  Timestamp = Struct.new(:sec, :nsec)

  class Timestamp # a.k.a. "TimeSpec"
    TYPE = -1 # timestamp extension type

    TIMESTAMP32_MAX_SEC = (1 << 32) - 1
    TIMESTAMP64_MAX_SEC = (1 << 34) - 1

    def self.from_msgpack_ext(data)
      case data.length
      when 4
        # timestamp32 (sec: uint32be)
        sec = data.unpack1('L>')
        new(sec, 0)
      when 8
        # timestamp64 (nsec: uint30be, sec: uint34be)
        n, s = data.unpack('L>2')
        sec = ((n & 0b11) << 32) | s
        nsec = n >> 2
        new(sec, nsec)
      when 12
        # timestam96 (nsec: uint32be, sec: int64be)
        nsec, sec = data.unpack('L>q>')
        new(sec, nsec)
      else
        raise "Invalid timestamp data size: #{data.length}"
      end
    end

    def self.to_msgpack_ext(sec, nsec)
      if sec >= 0 && nsec >= 0 && sec <= TIMESTAMP64_MAX_SEC
        if nsec === 0 && sec <= TIMESTAMP32_MAX_SEC
          # timestamp32 = (sec: uint32be)
          [sec].pack('L>')
        else
          # timestamp64 (nsec: uint30be, sec: uint34be)
          sec_high = sec << 32
          sec_low = sec & 0xffffffff
          [(nsec << 2) | (sec_high & 0b11), sec_low].pack('L>2')
        end
      else
        # timestamp96 (nsec: uint32be, sec: int64be)
        [nsec, sec].pack('L>q>')
      end
    end

    def to_msgpack_ext
      self.class.to_msgpack_ext(sec, nsec)
    end
  end
end
