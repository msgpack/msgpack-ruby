require 'spec_helper'
require 'random_compat'

RSpec.describe Buffer do
  STATIC_EXAMPLES = {}
  STATIC_EXAMPLES[:empty01] = ''
  STATIC_EXAMPLES[:empty02] = ''
  STATIC_EXAMPLES[:copy01] = 'short'
  STATIC_EXAMPLES[:copy02] = 'short'*2
  STATIC_EXAMPLES[:ref01] = 'short'*128
  STATIC_EXAMPLES[:ref02] = 'short'*128*2
  STATIC_EXAMPLES[:ref03] = 'a'*((1024*1024+2)*2)
  STATIC_EXAMPLES[:refcopy01] = 'short'*128
  STATIC_EXAMPLES[:expand01] = 'short'*1024
  STATIC_EXAMPLES[:expand02] = 'short'*(127+1024)
  STATIC_EXAMPLES[:offset01] = 'ort'+'short'
  STATIC_EXAMPLES[:offset02] = 'ort'+'short'*127
  STATIC_EXAMPLES[:offset03] = 'ort'+'short'*(126+1024)

  if ''.respond_to?(:force_encoding)
    STATIC_EXAMPLES.each_value {|v| v.force_encoding('ASCII-8BIT') }
  end
  STATIC_EXAMPLES.each_value {|v| v.freeze }

  r = Random.new
  random_seed = r.seed
  puts "buffer random seed: 0x#{random_seed.to_s(16)}"

  let :random_cases_examples do
    r = Random.new(random_seed)
    cases = {}
    examples = {}

    10.times do |i|
      b = Buffer.new
      s = r.bytes(0)
      r.rand(3).times do
        n = r.rand(1024*1400)
        x = r.bytes(n)
        s << x
        b << x
      end
      r.rand(2).times do
        n = r.rand(1024*1400)
        b.read(n)
        s.slice!(0, n)
      end
      key = :"random#{"%02d"%i}"
      cases[key] = b
      examples[key] = s
    end

    [cases, examples]
  end

  let :static_cases do
    map = {}
    map[:empty01] = empty01
    map[:empty02] = empty02
    map[:copy01] = copy01
    map[:copy02] = copy02
    map[:ref01] = ref01
    map[:ref02] = ref02
    map[:ref03] = ref03
    map[:refcopy01] = refcopy01
    map[:expand01] = expand01
    map[:expand02] = expand02
    map[:offset01] = offset01
    map[:offset02] = offset02
    map[:offset03] = offset03
    map
  end

  let :static_examples do
    STATIC_EXAMPLES
  end

  let :random_cases do
    random_cases_examples[0]
  end

  let :random_examples do
    random_cases_examples[1]
  end

  let :cases do
    static_cases.merge(random_cases)
  end

  let :examples do
    static_examples.merge(random_examples)
  end

  let :case_keys do
    examples.keys
  end

  let :empty01 do
    Buffer.new
  end

  let :empty02 do
    b = Buffer.new
    b << 'a'
    b.read_all(1)
    b
  end

  let :copy01 do
    # one copy chunk
    b = Buffer.new
    b << 'short'
    b
  end

  let :copy02 do
    # one copy chunk
    b = Buffer.new
    b << 'short'
    b << 'short'
    b
  end

  let :expand02 do
    # one copy chunk / expanded
    b = Buffer.new
    1024.times do
      b << 'short'
    end
    b
  end

  let :ref01 do
    # one reference chunk
    b = Buffer.new
    b << 'short'*128
    b.to_s
    b
  end

  let :ref02 do
    # two reference chunks
    b = Buffer.new
    b << 'short'*128
    b.to_s
    b << 'short'*128
    b.to_a
    b
  end

  let :ref03 do
    # two reference chunks
    b = Buffer.new
    b << 'a'*(1024*1024+2)
    b << 'a'*(1024*1024+2)
    b
  end

  let :refcopy01 do
    # one reference chunk + one copy chunk
    b = Buffer.new
    b << 'short'*127
    b.to_s
    b << 'short'
    b
  end

  let :expand01 do
    # one copy chunk / expanded
    b = Buffer.new
    1024.times do
      b << 'short'
    end
    b
  end

  let :expand02 do
    # one reference chunk + one copy chunk / expanded
    b = Buffer.new
    b << 'short'*127
    b.to_s
    1024.times do
      b << 'short'
    end
    b
  end

  let :offset01 do
    # one copy chunk / offset
    b = Buffer.new
    b << 'short'
    b << 'short'
    b.skip(2)
    b
  end

  let :offset02 do
    # one reference chunk / offset
    b = Buffer.new
    b << 'short'*127
    b.to_s
    b.skip(2)
    b << 'short'
    b
  end

  let :offset03 do
    # one reference chunk / offset + one copy chunk / expanded
    b = Buffer.new
    b << 'short'*127
    b.to_s
    1024.times do
      b << 'short'
    end
    b.skip(2)
    b
  end

  it 'empty?' do
    expect(empty01.empty?).to eq true
    expect(empty02.empty?).to eq true
  end

  it 'size' do
    case_keys.each {|k|
      expect(cases[k].size).to eq examples[k].size
    }
  end

  it 'short write increments size' do
    case_keys.each {|k|
      sz = examples[k].size
      10.times do |i|
        cases[k].write 'short'
        sz += 'short'.size
        expect(cases[k].size).to eq sz
      end
    }
  end

  it 'read with limit decrements size' do
    case_keys.each {|k|
      sz = examples[k].size
      next if sz < 2

      expect(cases[k].read(1)).to eq examples[k][0,1]
      sz -= 1
      expect(cases[k].size).to eq sz

      expect(cases[k].read(1)).to eq examples[k][1,1]
      sz -= 1
      expect(cases[k].size).to eq sz
    }
  end

  it 'read_all with limit decrements size' do
    case_keys.each {|k|
      sz = examples[k].size
      next if sz < 2

      expect(cases[k].read_all(1)).to eq examples[k][0,1]
      sz -= 1
      expect(cases[k].size).to eq sz

      expect(cases[k].read_all(1)).to eq examples[k][1,1]
      sz -= 1
      expect(cases[k].size).to eq sz
    }
  end

  it 'skip decrements size' do
    case_keys.each {|k|
      sz = examples[k].size
      next if sz < 2

      expect(cases[k].skip(1)).to eq 1
      sz -= 1
      expect(cases[k].size).to eq sz

      expect(cases[k].skip(1)).to eq 1
      sz -= 1
      expect(cases[k].size).to eq sz
    }
  end

  it 'skip_all decrements size' do
    case_keys.each {|k|
      sz = examples[k].size
      next if sz < 2

      cases[k].skip_all(1)
      sz -= 1
      expect(cases[k].size).to eq sz

      cases[k].skip_all(1)
      sz -= 1
      expect(cases[k].size).to eq sz
    }
  end

  it 'read_all against insufficient buffer raises EOFError and consumes nothing' do
    case_keys.each {|k|
      sz = examples[k].size
      expect { cases[k].read_all(sz+1) }.to raise_error(EOFError)
      expect(cases[k].size).to eq sz
    }
  end

  it 'skip_all against insufficient buffer raises EOFError and consumes nothing' do
    case_keys.each {|k|
      sz = examples[k].size
      expect { cases[k].skip_all(sz+1) }.to raise_error(EOFError)
      expect(cases[k].size).to eq sz
    }
  end

  it 'read against insufficient buffer consumes all buffer or return nil' do
    case_keys.each {|k|
      sz = examples[k].size
      if sz == 0
        expect(cases[k].read(sz+1)).to eq nil
      else
        expect(cases[k].read(sz+1)).to eq examples[k]
      end
      expect(cases[k].size).to eq 0
    }
  end

  it 'skip against insufficient buffer consumes all buffer' do
    case_keys.each {|k|
      sz = examples[k].size
      expect(cases[k].skip(sz+1)).to eq examples[k].size
      expect(cases[k].size).to eq 0
    }
  end

  it 'read with no arguments consumes all buffer and returns string and do not return nil' do
    case_keys.each {|k|
      expect(cases[k].read_all).to eq examples[k]
      expect(cases[k].size).to eq 0
    }
  end

  it 'read_all with no arguments consumes all buffer and returns string' do
    case_keys.each {|k|
      expect(cases[k].read_all).to eq examples[k]
      expect(cases[k].size).to eq 0
    }
  end

  it 'to_s returns a string and consume nothing' do
    case_keys.each {|k|
      expect(cases[k].to_s).to eq examples[k]
      expect(cases[k].size).to eq examples[k].size
    }
  end

  it 'to_s and modify' do
    case_keys.each {|k|
      s = cases[k].to_s
      s << 'x'
      expect(cases[k].to_s).to eq examples[k]
    }
  end

  it 'big write and modify reference' do
    big2 = "a" * (1024*1024 + 2)

    case_keys.each {|k|
      big1 = "a" * (1024*1024 + 2)
      cases[k].write(big1)
      big1 << 'x'
      expect(cases[k].read).to eq examples[k] + big2
    }
  end

  it 'big write -> short write' do
    big1 = "a" * (1024*1024 + 2)

    case_keys.each {|k|
      sz = examples[k].size

      cases[k].write big1
      expect(cases[k].size).to eq sz + big1.size

      cases[k].write("c")
      expect(cases[k].size).to eq sz + big1.size + 1

      expect(cases[k].read_all).to eq examples[k] + big1 + "c"
      expect(cases[k].size).to eq 0
      expect(cases[k].empty?).to eq true
    }
  end

  it 'big write 2'do
    big1 = "a" * (1024*1024 + 2)
    big2 = "b" * (1024*1024 + 2)

    case_keys.each {|k|
      sz = examples[k].size

      cases[k].write big1
      cases[k].write big2
      expect(cases[k].size).to eq sz + big1.size + big2.size

      expect(cases[k].read_all(sz + big1.size)).to eq examples[k] + big1
      expect(cases[k].size).to eq big2.size

      expect(cases[k].read).to eq big2
      expect(cases[k].size).to eq 0
      expect(cases[k].empty?).to eq true
    }
  end

  it 'read 0' do
    case_keys.each {|k|
      expect(cases[k].read(0)).to eq ''
      expect(cases[k].read).to eq examples[k]
    }
  end

  it 'skip 0' do
    case_keys.each {|k|
      expect(cases[k].skip(0)).to eq 0
      expect(cases[k].read).to eq examples[k]
    }
  end

  it 'read_all 0' do
    case_keys.each {|k|
      expect(cases[k].read_all(0)).to eq ''
      expect(cases[k].read_all).to eq examples[k]
    }
  end

  it 'skip_all 0' do
    case_keys.each {|k|
      cases[k].skip_all(0)
      expect(cases[k].read_all).to eq examples[k]
    }
  end

  it 'write_to' do
    case_keys.each {|k|
      sio = StringIO.new
      expect(cases[k].write_to(sio)).to eq examples[k].size
      expect(cases[k].size).to eq 0
      expect(sio.string).to eq examples[k]
    }
  end

  it 'random read/write' do
    r = Random.new(random_seed)
    s = r.bytes(0)
    b = Buffer.new

    10.times {
      # write
      r.rand(4).times do
        n = r.rand(1024*1400)
        x = r.bytes(n)
        s << x
        b.write(x)
      end

      # read
      r.rand(3).times do
        n = r.rand(1024*1400)
        ex = s.slice!(0, n)
        ex = nil if ex.empty?
        x = b.read(n)
        x.size == ex.size if x != nil
        expect(x).to eq ex
        expect(b.size).to eq s.size
      end
    }
  end

  it 'random read_all/write' do
    r = Random.new(random_seed)
    s = r.bytes(0)
    b = Buffer.new

    10.times {
      # write
      r.rand(4).times do
        n = r.rand(1024*1400)
        x = r.bytes(n)
        s << x
        b.write(x)
      end

      # read_all
      r.rand(3).times do
        n = r.rand(1024*1400)
        begin
          x = b.read_all(n)
          ex = s.slice!(0, n)
          x.size == n
          expect(x).to eq ex
          expect(b.size).to eq s.size
        rescue EOFError
          expect(b.size).to eq s.size
          expect(b.read).to eq s
          s.clear
          break
        end
      end
    }
  end

  it 'random skip write' do
    r = Random.new(random_seed)
    s = r.bytes(0)
    b = Buffer.new

    10.times {
      # write
      r.rand(4).times do
        n = r.rand(1024*1400)
        x = r.bytes(n)
        s << x
        b.write(x)
      end

      # skip
      r.rand(3).times do
        n = r.rand(1024*1400)
        ex = s.slice!(0, n)
        expect(b.skip(n)).to eq ex.size
        expect(b.size).to eq s.size
      end
    }
  end

  it 'random skip_all write' do
    r = Random.new(random_seed)
    s = r.bytes(0)
    b = Buffer.new

    10.times {
      # write
      r.rand(4).times do
        n = r.rand(1024*1400)
        x = r.bytes(n)
        s << x
        b.write(x)
      end

      # skip_all
      r.rand(3).times do
        n = r.rand(1024*1400)
        begin
          b.skip_all(n)
          s.slice!(0, n)
          expect(b.size).to eq s.size
        ensure EOFError
          expect(b.size).to eq s.size
          expect(b.read).to eq s
          s.clear
          break
        end
      end
    }
  end
end
