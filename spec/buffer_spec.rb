require 'spec_helper'

describe Buffer do
  EXAMPLES = {}
  EXAMPLES[:empty01] = ''
  EXAMPLES[:empty02] = ''
  EXAMPLES[:copy01] = 'short'
  EXAMPLES[:copy02] = 'short'*2
  EXAMPLES[:ref01] = 'short'*128
  EXAMPLES[:ref02] = 'short'*128*2
  EXAMPLES[:ref03] = 'a'*((1024*1024+2)*2)
  EXAMPLES[:refcopy01] = 'short'*128
  EXAMPLES[:expand01] = 'short'*1024
  EXAMPLES[:expand02] = 'short'*(127+1024)
  EXAMPLES[:offset01] = 'ort'+'short'
  EXAMPLES[:offset02] = 'ort'+'short'*127
  EXAMPLES[:offset03] = 'ort'+'short'*(126+1024)

  if ''.respond_to?(:force_encoding)
    EXAMPLES.each_value {|v| v.force_encoding('ASCII-8BIT') }
  end
  EXAMPLES.each_value {|v| v.freeze }

  let :examples do
    EXAMPLES
  end

  let :cases do
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
    empty01.empty?.should == true
    empty02.empty?.should == true
  end

  it 'size' do
    case_keys.each {|k|
      cases[k].size.should == examples[k].size
    }
  end

  it 'short append increments size' do
    case_keys.each {|k|
      sz = examples[k].size
      10.times do |i|
        cases[k].append 'short'
        sz += 'short'.size
        cases[k].size.should == sz
      end
    }
  end

  it 'read with limit decrements size' do
    case_keys.each {|k|
      sz = examples[k].size
      next if sz < 2

      cases[k].read(1).should == examples[k][0,1]
      sz -= 1
      cases[k].size.should == sz

      cases[k].read(1).should == examples[k][1,1]
      sz -= 1
      cases[k].size.should == sz
    }
  end

  it 'read_all with limit decrements size' do
    case_keys.each {|k|
      sz = examples[k].size
      next if sz < 2

      cases[k].read_all(1).should == examples[k][0,1]
      sz -= 1
      cases[k].size.should == sz

      cases[k].read_all(1).should == examples[k][1,1]
      sz -= 1
      cases[k].size.should == sz
    }
  end

  it 'skip decrements size' do
    case_keys.each {|k|
      sz = examples[k].size
      next if sz < 2

      cases[k].skip(1).should == 1
      sz -= 1
      cases[k].size.should == sz

      cases[k].skip(1).should == 1
      sz -= 1
      cases[k].size.should == sz
    }
  end

  it 'skip_all decrements size' do
    case_keys.each {|k|
      sz = examples[k].size
      next if sz < 2

      cases[k].skip_all(1)
      sz -= 1
      cases[k].size.should == sz

      cases[k].skip_all(1)
      sz -= 1
      cases[k].size.should == sz
    }
  end

  it 'read_all against insufficient buffer raises EOFError and consumes nothing' do
    case_keys.each {|k|
      sz = examples[k].size
      lambda {
        cases[k].read_all(sz+1)
      }.should raise_error(EOFError)
      cases[k].size.should == sz
    }
  end

  it 'skip_all against insufficient buffer raises EOFError and consumes nothing' do
    case_keys.each {|k|
      sz = examples[k].size
      lambda {
        cases[k].skip_all(sz+1)
      }.should raise_error(EOFError)
      cases[k].size.should == sz
    }
  end

  it 'read against insufficient buffer consumes all buffer or return nil' do
    case_keys.each {|k|
      sz = examples[k].size
      if sz == 0
        cases[k].read(sz+1).should == nil
      else
        cases[k].read(sz+1).should == examples[k]
      end
      cases[k].size.should == 0
    }
  end

  it 'skip against insufficient buffer consumes all buffer' do
    case_keys.each {|k|
      sz = examples[k].size
      cases[k].skip(sz+1).should == examples[k].size
      cases[k].size.should == 0
    }
  end

  it 'read with no arguments consumes all buffer and returns string and do not return nil' do
    case_keys.each {|k|
      cases[k].read_all.should == examples[k]
      cases[k].size.should == 0
    }
  end

  it 'read_all with no arguments consumes all buffer and returns string' do
    case_keys.each {|k|
      cases[k].read_all.should == examples[k]
      cases[k].size.should == 0
    }
  end

  it 'to_s returns a string and consume nothing' do
    case_keys.each {|k|
      cases[k].to_s.should == examples[k]
      cases[k].size.should == examples[k].size
    }
  end

  it 'to_s and modify' do
    case_keys.each {|k|
      s = cases[k].to_s
      s << 'x'
      cases[k].to_s.should == examples[k]
    }
  end

  it 'big append and modify reference' do
    big2 = "a" * (1024*1024 + 2)

    case_keys.each {|k|
      big1 = "a" * (1024*1024 + 2)
      cases[k].append(big1)
      big1 << 'x'
      cases[k].read.should == examples[k] + big2
    }
  end

  it 'big append -> short append' do
    biglen = 1024*1024 + 2
    big1 = "a" * (1024*1024 + 2)

    case_keys.each {|k|
      sz = examples[k].size

      cases[k].append big1
      cases[k].size.should == sz + big1.size

      cases[k].append("c")
      cases[k].size.should == sz + big1.size + 1

      cases[k].read_all.should == examples[k] + big1 + "c"
      cases[k].size.should == 0
      cases[k].empty?.should == true
    }
  end

  it 'big append 2'do
    big1 = "a" * (1024*1024 + 2)
    big2 = "b" * (1024*1024 + 2)

    case_keys.each {|k|
      sz = examples[k].size

      cases[k].append big1
      cases[k].append big2
      cases[k].size.should == sz + big1.size + big2.size

      cases[k].read_all(sz + big1.size).should == examples[k] + big1
      cases[k].size.should == big2.size

      cases[k].read.should == big2
      cases[k].size.should == 0
      cases[k].empty?.should == true
    }
  end

  it 'read 0' do
    case_keys.each {|k|
      cases[k].read(0).should == ''
      cases[k].read.should == examples[k]
    }
  end

  it 'skip 0' do
    case_keys.each {|k|
      cases[k].skip(0).should == 0
      cases[k].read.should == examples[k]
    }
  end

  it 'read_all 0' do
    case_keys.each {|k|
      cases[k].read_all(0).should == ''
      cases[k].read_all.should == examples[k]
    }
  end

  it 'skip_all 0' do
    case_keys.each {|k|
      cases[k].skip_all(0)
      cases[k].read_all.should == examples[k]
    }
  end

  it 'random read/write' do
    r = Random.new
    s = r.bytes(0)
    b = Buffer.new

    puts "random read/write seed: 0x#{r.seed.to_s(16)}"
    100.times {
      # write
      r.rand(3).times do
        n = r.rand(1024*1400)
        x = r.bytes(n)
        s << x
        b << x
      end

      # read
      r.rand(2).times do
        n = r.rand(1024*1400)
        ex = s.slice!(0, n)
        ex = nil if ex.empty?
        b.read(n).should == ex
        b.size.should == s.size
      end
    }
  end

  it 'random skip write' do
    r = Random.new
    s = r.bytes(0)
    b = Buffer.new

    puts "random skip/write seed: 0x#{r.seed.to_s(16)}"
    100.times {
      # write
      r.rand(3).times do
        n = r.rand(1024*1400)
        x = r.bytes(n)
        s << x
        b << x
      end

      # skip
      r.rand(2).times do
        n = r.rand(1024*1400)
        ex = s.slice!(0, n)
        b.skip(n).should == ex.size
        b.size.should == s.size
      end
    }
  end
end

