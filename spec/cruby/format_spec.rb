# encoding: ascii-8bit
require 'spec_helper'

describe MessagePack do

  it "fixext 1" do
    check_ext 2, 1, -128
    check_ext 2, 1, 1
    check_ext 2, 1, 127
  end

  it "fixext 2" do
    check_ext 2, 2, -128
    check_ext 2, 2, 1
    check_ext 2, 2, 127
  end

  it "fixext 4" do
    check_ext 2, 4, -128
    check_ext 2, 4, 1
    check_ext 2, 4, 127
  end

  it "fixext 8" do
    check_ext 2, 8, -128
    check_ext 2, 8, 1
    check_ext 2, 8, 127
  end

  it "fixext 16" do
    check_ext 2, 16, -128
    check_ext 2, 16, 1
    check_ext 2, 16, 127
  end


  it "ext 8" do
    check_ext 3, (1<<8) - 1, -128
    check_ext 3, (1<<8) - 1, 1
    check_ext 3, (1<<8) - 2, 127
  end

  it "ext 16" do
    check_ext 4, (1<<16) - 1, -128
    check_ext 4, (1<<8), 1
    check_ext 4, (1<<16) - 2, 127
  end

  it "ext 32" do
    check_ext 6, (1<<20), -128
    check_ext 6, (1<<16), 1
    check_ext 6, (1<<16), 127
  end

  it "extended type 1 with payload aa" do
    obj = MessagePack::ExtensionValue.new(1, "aa")
    match obj, "\xd5\x01aa"
  end

  it "extended type 1 with payload aaaa" do
    obj = MessagePack::ExtensionValue.new(1, "aaaa")
    match obj, "\xd6\x01aaaa"
  end

  it "extended type 1 with payload aaaa" do
    obj = MessagePack::ExtensionValue.new(1, "aaaa")
    match obj, "\xd6\x01aaaa"
  end

  it "extended type 1 with payload aaaaaaaa" do
    obj = MessagePack::ExtensionValue.new(1, "aaaaaaaa")
    match obj, "\xd7\x01aaaaaaaa"
  end

  it "extended type 1 with a payload of 2^8 - 1 bytes" do
    size = (1<<8) - 1
    obj = MessagePack::ExtensionValue.new(1, "a" * size)
    match obj, "\xc7\xff\x01" << ("a" * size)
  end

  it "extended type 1 with a payload of 2^16 - 1 bytes" do
    size = (1<<16) - 1
    obj = MessagePack::ExtensionValue.new(1, "a" * size)
    match obj, "\xc8\xff\xff\x01" << ("a" * size)
  end

  it "extended type 1 with a payload of 2^16 - 2 bytes" do
    size = (1<<16) - 2
    obj = MessagePack::ExtensionValue.new(1, "a" * size)
    match obj, "\xc8\xff\xfe\x01" << ("a" * size)
  end

  it "extended type 1 with a payload of 2^16" do
    size = (1<<16)
    obj = MessagePack::ExtensionValue.new(1, "a" * size)
    match obj, "\xc9\x00\x01\x00\x00\x01" << ("a" * size)
  end

  it "extended type 1 with a payload of 2^16 + 1" do
    size = (1<<16) + 1
    obj = MessagePack::ExtensionValue.new(1, "a" * size)
    match obj, "\xc9\x00\x01\x00\x01\x01" << ("a" * size)
  end

  def check(len, obj)
    raw = obj.to_msgpack.to_s
    raw.length.should == len
    MessagePack.unpack(raw).should == obj
  end

  def check_ext(overhead, num, type)
    check num+overhead, MessagePack::ExtensionValue.new(type, "a" * num)
  end

  def match(obj, buf)
    raw = obj.to_msgpack.to_s
    raw.should == buf
  end
end
