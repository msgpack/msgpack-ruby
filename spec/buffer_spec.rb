require 'spec_helper'

describe Buffer do
  it 'short append increments size' do
    b = Buffer.new
    b.size.should == 0
    10.times do |i|
      b.append 'short'
      b.size.should == 'short'.size * (i+1)
    end
  end

  it 'empty?' do
    b = Buffer.new

    b.empty?.should == true

    b.append 'a'
    b.empty?.should == false
  end

  it 'read with limit decrements size' do
    b = Buffer.new

    b.append 'ch'

    b.read(1).should == 'c'
    b.size.should == 1
    b.read(1).should == 'h'
    b.size.should == 0
  end

  it 'skip decrements size' do
    b = Buffer.new

    b.append 'ch'

    b.skip(1)
    b.size.should == 1
    b.skip(1)
    b.size.should == 0
  end

  it 'skip_all against the insufficient buffer raises EOFError' do
    b = Buffer.new

    lambda {
      b.skip_all(1)
    }.should raise_error(EOFError)

    b.append 'c'

    lambda {
      b.skip_all(2)
    }.should raise_error(EOFError)

    b.size.should == 1

    b.skip_all(1)

    lambda {
      b.skip_all(1)
    }.should raise_error(EOFError)
  end

  it 'read_all with limit decrements size' do
    b = Buffer.new

    b.append 'ch'

    b.read_all(1).should == 'c'
    b.size.should == 1
    b.read_all(1).should == 'h'
    b.size.should == 0
  end

  it 'read_all against the insufficient buffer raises EOFError' do
    b = Buffer.new

    lambda {
      b.read_all(1)
    }.should raise_error(EOFError)

    b.append 'c'

    lambda {
      b.read_all(2)
    }.should raise_error(EOFError)

    b.size.should == 1

    b.read_all(1).should == 'c'

    lambda {
      b.read_all(1)
    }.should raise_error(EOFError)

    b.read_all.should == ''
  end

  it 'big append 1' do
    biglen = 1024*1024 + 2
    big1 = "a" * biglen

    b = Buffer.new

    b.append(big1)
    b.size == biglen

    b.read_all.should == big1
    b.size.should == 0

    b.append("c")
    b.size.should == 1

    b.read_all.should == "c"
    b.size.should == 0
  end

  it 'big append 2' do
    biglen = 1024*1024 + 2
    big1 = "a" * biglen
    big2 = "b" * biglen

    b = Buffer.new

    b.append(big1)
    b.size.should == biglen

    b.append(big2)
    b.size.should == biglen * 2

    b.read_all(1).should == "a"
    b.size.should == biglen * 2 - 1

    b.read_all(biglen-1).should == "a" * (biglen-1)
    b.size.should == biglen

    b.read_all(biglen).should == big2
    b.size.should == 0
  end

  it 'big append and read_all' do
    biglen = 1024*1024 + 2
    halflen = biglen / 2
    big1 = "a" * biglen
    big2 = "b" * biglen
    big3 = "c" * biglen
    big3_half = "c" * halflen

    b = Buffer.new

    b.append(big1)
    b.size.should == biglen

    b.append(big2)
    b.size.should == biglen * 2

    b.append(big3)
    b.size.should == biglen * 3

    # consume part of a chunk
    b.read_all(1).should == 'a'
    b.size.should == biglen * 3 - 1

    # consume just a chunk
    b.read_all(biglen-1).should == 'a'*(biglen-1)
    b.size.should == biglen * 2

    # consume a chunk + half
    b.read_all(biglen + halflen).should == big2 + big3_half
    b.size.should == halflen

    # consume just a chunk
    b.read_all(halflen).should == big3_half
    b.size.should == 0
  end

  it 'big append and short append' do
    biglen = 1024*1024 + 2
    big1 = "a" * biglen

    b = Buffer.new

    b.append(big1)

    b.append("c")
    b.size.should == biglen + 1

    b.read_all(1)
    b.size.should == biglen

    b.append("c")
    b.size.should == biglen + 1
  end

  it 'big append and modify reference' do
    biglen = 1024*1024 + 2
    big1 = "a" * biglen

    b = Buffer.new

    b.append(big1)
    big1[0] = 'b'

    b.read_all.should == "a" * biglen
  end

  it 'short append and to_s' do
    b = Buffer.new

    b.to_s.should == ''

    b.append 'short'
    b.to_s.should == 'short'
    b.to_s.should == 'short'

    b.append 'short'
    b.to_s.should == 'shortshort'
    b.to_s.should == 'shortshort'
  end

  it 'short append and to_s which may move' do
    b = Buffer.new
    n = 100

    n.times do
      b.append 'short'
    end

    b.to_s.should == 'short' * n
  end

  it 'to_s and modify' do
    b = Buffer.new

    b.append 'short'
    s = b.to_s

    s[0] = 'x'

    b.to_s.should == 'short'
  end

  it 'big append and to_s' do
    biglen = 1024*1024 + 2
    big1 = "a" * biglen
    big2 = "b" * biglen

    b = Buffer.new

    b.append big1
    b.to_s.should == big1
    b.to_s.should == big1
    b.append big2
    b.to_s.should == big1 + big2
    b.to_s.should == big1 + big2
  end

  it 'big append and to_s' do
    biglen = 1024*1024 + 2
    big1 = "a" * biglen
    n = 1024

    b = Buffer.new

    n.times do
      b.append big1
    end
    b.size.should == biglen * n
    b.to_s.size.should == biglen * n
  end
end

