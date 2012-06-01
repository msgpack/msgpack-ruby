# encoding: ascii-8bit
require 'spec_helper'
require 'stringio'

if defined?(Encoding)
  Encoding.default_internal = 'ASCII-8BIT'
  Encoding.default_external = 'ASCII-8BIT'
end

describe Packer do
  let :packer do
    Packer.new
  end

  it 'initialize' do
    Packer.new
    Packer.new(nil)
    Packer.new(StringIO.new)
    Packer.new({})
    Packer.new(StringIO.new, {})
  end

  it 'Packer' do
    Packer(packer).object_id.should == packer.object_id
    Packer(nil).class.should == Packer
    Packer('').class.should == Packer
    Packer('initbuf').to_s.should == 'initbuf'
  end

  it 'write' do
    packer.write([])
    packer.to_s.should == "\x90"
  end

  it 'write_nil' do
    packer.write_nil
    packer.to_s.should == "\xc0"
  end

  it 'write_array_header 0' do
    packer.write_array_header(0)
    packer.to_s.should == "\x90"
  end

  it 'write_array_header 1' do
    packer.write_array_header(1)
    packer.to_s.should == "\x91"
  end

  it 'write_map_header 0' do
    packer.write_map_header(0)
    packer.to_s.should == "\x80"
  end

  it 'write_map_header 1' do
    packer.write_map_header(1)
    packer.to_s.should == "\x81"
  end

  it 'flush' do
    io = StringIO.new
    pk = Packer.new(io)
    pk.write_nil
    pk.flush
    pk.to_s.should == ''
    io.string.should == "\xc0"
  end

  it 'buffer' do
    o1 = packer.buffer.object_id
    packer.buffer << 'frsyuki'
    packer.buffer.to_s.should == 'frsyuki'
    packer.buffer.object_id.should == o1
  end
end

