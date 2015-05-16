# encoding: ascii-8bit
require 'spec_helper'

describe MessagePack do
  it "nil" do
    check nil => "\xc0"
  end

  it "Boolean" do
    check false => "\xc2", true => "\xc3"
  end

  describe "Integer" do
    it "positive fixnum" do
      check(
        0 => "\x00",
        1 => "\x01",
        (1<<7)-1 => "\x7f"
      )
    end

    it "negative fixnum" do
      check(
        -1 => "\xff",
        -(1<<5) => "\xe0"
      )
    end

    it "uint 8" do
      check(
        1<<7 => "\xcc\x80",
        (1<<8)-1 => "\xcc\xff"
      )
    end

    it "uint 16" do
      check(
        1<<8 => "\xcd\x01\x00",
        (1<<16)-1 => "\xcd\xff\xff"
      )
    end

    it "uint 32" do
      check(
        1<<16 => "\xce\x00\x01\x00\x00",
        (1<<32)-1 => "\xce\xff\xff\xff\xff"
      )
    end

    it "uint 64" do
      check(
        1<<32 => "\xcf\x00\x00\x00\x01\x00\x00\x00\x00",
        (1<<64)-1 => "\xcf\xff\xff\xff\xff\xff\xff\xff\xff"
      )
    end

    it "int 8" do
      check(
        -((1<<5)+1) => "\xd0\xdf",
        -(1<<7) => "\xd0\x80"
      )
    end

    it "int 16" do
      check(
        -((1<<7)+1) => "\xd1\xff\x7f",
        -(1<<15) => "\xd1\x80\x00"
      )
    end

    it "int 32" do
      check(
        -((1<<15)+1) => "\xd2\xff\xff\x7f\xff",
        -(1<<31) => "\xd2\x80\x00\x00\x00"
      )
    end

    it "int 64" do
      check(
        -((1<<31)+1) => "\xd3\xff\xff\xff\xff\x7f\xff\xff\xff",
        -(1<<63) => "\xd3\x80\x00\x00\x00\x00\x00\x00\x00"
      )
    end
  end

  describe "Float" do
    it "float 64" do
      check 1.0 => "\xcb\x3f\xf0\x00\x00\x00\x00\x00\x00"
    end
  end

  describe "String" do
    it "fixstr" do
      check(
        'hello'.encode('utf-8') => "\xa5hello",
        str_n(0) => "\xa0",
        s=str_n( (1<<5)-1 ) => "\xbf#{s}"
      )
    end

    it "str 8" do
      check(
        s=str_n( 1<<5 ) => "\xd9\x20#{s}",
        s=str_n( (1<<8)-1 ) => "\xd9\xff#{s}"
      )
    end

    it "str 16" do
      check(
        s=str_n( 1<<8 ) => "\xda\x01\x00#{s}",
        s=str_n( (1<<16)-1 ) => "\xda\xff\xff#{s}"
      )
    end

    it "str 32" do
      check( s=str_n( 1<<16 ) => "\xdb\x00\x01\x00\x00#{s}" )
    end
  end

  describe "Array" do
    it "fixarray" do
      check(
        array_n(0) => "\x90",
        array_n( n=(1<<4)-1 ) => "\x9f" + ("\x2a"*n)
      )
    end

    it "array 16" do
      check(
        array_n( n=1<<4 ) => "\xdc\x00\x10" + ("\x2a"*n),
        array_n( n=(1<<16)-1 ) => "\xdc\xff\xff" + ("\x2a"*n)
      )
    end

    it "array 32" do
      check( array_n( n=1<<16 ) => "\xdd\x00\x01\x00\x00" + ("\x2a"*n) )
    end
  end

  describe "Map" do
    it "fixmap" do
      check(
        {} => "\x80",
        map_n(1) => "\x81" + map_n_packed(1),
        map_n( n=(1<<4)-1 ) => "\x8f" + map_n_packed(n)
      )
    end

    it "map 16" do
      check(
        map_n( n=1<<4 ) => "\xde\x00\x10" + map_n_packed(n),
        map_n( n=(1<<16)-1 ) => "\xde\xff\xff" + map_n_packed(n)
      )
    end

    it "map 32" do
      check( map_n( n=1<<16 ) => "\xdf\x00\x01\x00\x00" + map_n_packed(n) )
    end
  end

  def check(map)
    map.each do |native, packed|
      expect(MessagePack.pack(native)).to eq(packed)

      unpacked = MessagePack.unpack(packed)
      expect(unpacked).to eq(native)

      expect(native.encoding).to eq(unpacked.encoding)  if native.is_a?(String)
    end
  end

  def str_n(n)
    ('X' * n).encode('utf-8')
  end

  def array_n(n)
    [42] * n
  end

  def map_n(n)
    Hash[ (0...n).map { |v|  [v, 42] } ]
  end

  def map_n_packed(n)
    packed = ''
    (0...n).each { |i|  packed += i.to_msgpack + 42.to_msgpack }
    packed
  end
end

