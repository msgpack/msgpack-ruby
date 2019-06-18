# frozen_string_literal: true

module MessagePack
  Timestamp = Struct.new(:sec, :nsec)

  class Timestamp # a.k.a. "TimeSpec"
    TYPE = -1 # timestamp extension type

    TIMESTAMP32_MAX_SEC = 0x100000000 # 32-bit unsigned int
    TIMESTAMP64_MAX_SEC = 0x400000000 # 34-bit unsigned int

    def self.from_msgpack_ext(data)
      case data.length
      when 4
        # timestamp32 = (uint32be)
        sec = data.unpack1('L>')
        new(sec, 0)
      when 8
        # timestamp64 = (uint32be, uint32be)
        n, s = data.unpack('L>2')
        sec = (n & 0x3) * 0x100000000 + s
        nsec = n >> 2
        new(sec, nsec)
      when 12
        # timestam96 = (uint32be, int64be)
        nsec, sec = data.unpack('L>q>')
        new(sec, nsec)
      else
        raise "Invalid timestamp data size: #{data.length}"
      end
    end

    def self.to_msgpack_ext(sec, nsec)
      if sec >= 0 && nsec >= 0 && sec < TIMESTAMP64_MAX_SEC
        if nsec === 0 && sec < TIMESTAMP32_MAX_SEC
          # timestamp32 = (uint32be)
          [sec].pack('L>')
        else
          # timestamp64 (uint32be, uint32be)
          sec_high = sec << 32
          sec_low = sec & 0xffffffff
          [(nsec << 2) | (sec_high & 0x3), sec_low].pack('L>2')
        end
      else
        # timestamp96 (uint32be, int64be)
        [nsec, sec].pack('L>q>')
      end
    end

    def to_msgpack_ext
      self.class.to_msgpack_ext(sec, nsec)
    end
  end
end
