# encoding: ascii-8bit
require 'spec_helper'

describe Unpacker do
  let :unpacker do
    Unpacker.new
  end

  # TODO read methods

  it "gc mark" do
    obj = [1024, {["a","b"]=>["c","d"]}, ["e","f"], "d", 70000, 4.12, 1.5, 1.5, 1.5]
    raw = obj.to_msgpack.to_s * 4

    n = 0
    raw.split(//).each do |b|
      GC.start
      unpacker.feed_each(b) {|o|
        GC.start
        o.should == obj
        n += 1
      }
      GC.start
    end

    n.should == 4
  end

  it "buffer" do
    orig = "a"*32*1024*4
    raw = orig.to_msgpack.to_s

    n = 655
    times = raw.size / n
    times += 1 unless raw.size % n == 0

    off = 0
    parsed = false

    times.times do
      parsed.should == false

      seg = raw[off, n]
      off += seg.length

      unpacker.feed_each(seg) {|obj|
        parsed.should == false
        obj.should == orig
        parsed = true
      }
    end

    parsed.should == true
  end
end

