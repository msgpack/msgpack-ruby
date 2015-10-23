require 'spec_helper'
require 'random_compat'

require 'stringio'
if defined?(Encoding)
  Encoding.default_external = 'ASCII-8BIT'
end

RSpec.describe Buffer do
  r = Random.new
  random_seed = r.seed
  puts "buffer_io random seed: 0x#{random_seed.to_s(16)}"

  let :source do
    ''
  end

  def set_source(s)
    source.replace(s)
  end

  let :io do
    StringIO.new(source.dup)
  end

  let :buffer do
    Buffer.new(io)
  end

  it 'io returns internal io' do
    expect(buffer.io).to eq io
  end

  it 'close closes internal io' do
    expect(io).to receive(:close)
    buffer.close
  end

  it 'short feed and read all' do
    set_source 'aa'
    expect(buffer.read).to eq 'aa'
  end

  it 'short feed and read short' do
    set_source 'aa'
    expect(buffer.read(1)).to eq 'a'
    expect(buffer.read(1)).to eq 'a'
    expect(buffer.read(1)).to eq nil
  end

  it 'long feed and read all' do
    set_source ' '*(1024*1024)
    s = buffer.read
    expect(s.size).to eq source.size
    expect(s).to eq source
  end

  it 'long feed and read mixed' do
    set_source ' '*(1024*1024)
    expect(buffer.read(10)).to eq source.slice!(0, 10)
    expect(buffer.read(10)).to eq source.slice!(0, 10)
    expect(buffer.read(10)).to eq source.slice!(0, 10)
    s = buffer.read
    expect(s.size).to eq source.size
    expect(s).to eq source
  end

  it 'eof' do
    set_source ''
    expect(buffer.read).to eq ''
  end

  it 'eof 2' do
    set_source 'a'
    expect(buffer.read).to eq 'a'
    expect(buffer.read).to eq ''
  end

  it 'write short once and flush' do
    buffer.write('aa')
    buffer.flush
    expect(io.string).to eq 'aa'
  end

  it 'write short twice and flush' do
    buffer.write('a')
    buffer.write('a')
    buffer.flush
    expect(io.string).to eq 'aa'
  end

  it 'write long once and flush' do
    s = ' '*(1024*1024)
    buffer.write s
    buffer.flush
    expect(io.string.size).to eq s.size
    expect(io.string).to eq s
  end

  it 'write short multi and flush' do
    s = ' '*(1024*1024)
    1024.times {
      buffer.write ' '*1024
    }
    buffer.flush
    expect(io.string.size).to eq s.size
    expect(io.string).to eq s
  end

  it 'random read' do
    r = Random.new(random_seed)

    50.times {
      fragments = []

      r.rand(4).times do
        n = r.rand(1024*1400)
        s = r.bytes(n)
        fragments << s
      end

      io = StringIO.new(fragments.join)
      b = Buffer.new(io)

      fragments.each {|s|
        x = b.read(s.size)
        expect(x.size).to eq s.size
        expect(x).to eq s
      }
      expect(b.empty?).to eq true
      expect(b.read).to eq ''
    }
  end

  it 'random read_all' do
    r = Random.new(random_seed)

    50.times {
      fragments = []
      r.bytes(0)

      r.rand(4).times do
        n = r.rand(1024*1400)
        s = r.bytes(n)
        fragments << s
      end

      io = StringIO.new(fragments.join)
      b = Buffer.new(io)

      fragments.each {|s|
        x = b.read_all(s.size)
        expect(x.size).to eq s.size
        expect(x).to eq s
      }
      expect(b.empty?).to eq true
      expect { b.read_all(1) }.to raise_error(EOFError)
    }
  end

  it 'random skip' do
    r = Random.new(random_seed)

    50.times {
      fragments = []

      r.rand(4).times do
        n = r.rand(1024*1400)
        s = r.bytes(n)
        fragments << s
      end

      io = StringIO.new(fragments.join)
      b = Buffer.new(io)

      fragments.each {|s|
        expect(b.skip(s.size)).to eq s.size
      }
      expect(b.empty?).to eq true
      expect(b.skip(1)).to eq 0
    }
  end

  it 'random skip_all' do
    r = Random.new(random_seed)

    50.times {
      fragments = []

      r.rand(4).times do
        n = r.rand(1024*1400)
        s = r.bytes(n)
        fragments << s
      end

      io = StringIO.new(fragments.join)
      b = Buffer.new(io)

      fragments.each {|s|
        expect { b.skip_all(s.size) }.not_to raise_error
      }
      expect(b.empty?).to eq true
      expect { b.skip_all(1) }.to raise_error(EOFError)
    }
  end

  it 'random write and flush' do
    r = Random.new(random_seed)

    50.times {
      s = r.bytes(0)
      io = StringIO.new
      b = Buffer.new(io)

      r.rand(4).times do
        n = r.rand(1024*1400)
        x = r.bytes(n)
        s << x
        b.write(x)
      end

      expect((io.string.size + b.size)).to eq s.size

      b.flush

      expect(io.string.size).to eq s.size
      expect(io.string).to eq s
    }
  end

  it 'random write and clear' do
    r = Random.new(random_seed)
    b = Buffer.new

    50.times {
      s = r.bytes(0)

      r.rand(4).times do
        n = r.rand(1024*1400)
        x = r.bytes(n)
        s << x
        b.write(x)
      end

      expect(b.size).to eq s.size
      b.clear
    }
  end
end
