# encoding: ascii-8bit

require 'stringio'
require 'tempfile'

require 'spec_helper'

describe MessagePack::Unpacker do
  let :unpacker do
    MessagePack::Unpacker.new
  end

  let :packer do
    MessagePack::Packer.new
  end

  # TODO initialize

  it 'read_array_header succeeds' do
    unpacker.feed("\x91")
    unpacker.read_array_header.should == 1
  end

  it 'read_array_header fails' do
    unpacker.feed("\x81")
    lambda {
      unpacker.read_array_header
    }.should raise_error(MessagePack::TypeError)  # TypeError is included in UnexpectedTypeError
    lambda {
      unpacker.read_array_header
    }.should raise_error(MessagePack::UnexpectedTypeError)
  end

  it 'read_map_header converts an map to key-value sequence' do
    packer.write_array_header(2)
    packer.write("e")
    packer.write(1)
    unpacker = MessagePack::Unpacker.new
    unpacker.feed(packer.to_s)
    unpacker.read_array_header.should == 2
    unpacker.read.should == "e"
    unpacker.read.should == 1
  end

  it 'read_map_header succeeds' do
    unpacker.feed("\x81")
    unpacker.read_map_header.should == 1
  end

  it 'read_map_header converts an map to key-value sequence' do
    packer.write_map_header(1)
    packer.write("k")
    packer.write("v")
    unpacker = MessagePack::Unpacker.new
    unpacker.feed(packer.to_s)
    unpacker.read_map_header.should == 1
    unpacker.read.should == "k"
    unpacker.read.should == "v"
  end

  it 'read_map_header fails' do
    unpacker.feed("\x91")
    lambda {
      unpacker.read_map_header
    }.should raise_error(MessagePack::TypeError)  # TypeError is included in UnexpectedTypeError
    lambda {
      unpacker.read_map_header
    }.should raise_error(MessagePack::UnexpectedTypeError)
  end

  it 'read raises EOFError' do
    lambda {
      unpacker.read
    }.should raise_error(EOFError)
  end

  let :sample_object do
    [1024, {["a","b"]=>["c","d"]}, ["e","f"], "d", 70000, 4.12, 1.5, 1.5, 1.5]
  end

  it 'feed and each continue internal state' do
    raw = sample_object.to_msgpack.to_s * 4
    objects = []

    raw.split(//).each do |b|
      unpacker.feed(b)
      unpacker.each {|c|
        objects << c
      }
    end

    objects.should == [sample_object] * 4
  end

  it 'feed_each continues internal state' do
    raw = sample_object.to_msgpack.to_s * 4
    objects = []

    raw.split(//).each do |b|
      unpacker.feed_each(b) {|c|
        objects << c
      }
    end

    objects.should == [sample_object] * 4
  end

  it 'feed_each enumerator' do
    raw = sample_object.to_msgpack.to_s * 4

    unpacker.feed_each(raw).to_a.should == [sample_object] * 4
  end

  it 'reset clears internal buffer' do
    # 1-element array
    unpacker.feed("\x91")
    unpacker.reset
    unpacker.feed("\x01")

    unpacker.each.map {|x| x }.should == [1]
  end

  it 'reset clears internal state' do
    # 1-element array
    unpacker.feed("\x91")
    unpacker.each.map {|x| x }.should == []

    unpacker.reset

    unpacker.feed("\x01")
    unpacker.each.map {|x| x }.should == [1]
  end

  it 'frozen short strings' do
    raw = sample_object.to_msgpack.to_s.force_encoding('UTF-8')
    lambda {
      unpacker.feed_each(raw.freeze) { }
    }.should_not raise_error
  end

  it 'frozen long strings' do
    raw = (sample_object.to_msgpack.to_s * 10240).force_encoding('UTF-8')
    lambda {
      unpacker.feed_each(raw.freeze) { }
    }.should_not raise_error
  end

  it 'read raises level stack too deep error' do
    512.times { packer.write_array_header(1) }
    packer.write(nil)

    unpacker = MessagePack::Unpacker.new
    unpacker.feed(packer.to_s)
    lambda {
      unpacker.read
    }.should raise_error(MessagePack::StackError)
  end

  it 'read raises invalid byte error' do
    unpacker.feed("\xc1")
    lambda {
      unpacker.read
    }.should raise_error(MessagePack::MalformedFormatError)
  end

  it "gc mark" do
    raw = sample_object.to_msgpack.to_s * 4

    n = 0
    raw.split(//).each do |b|
      GC.start
      unpacker.feed_each(b) {|o|
        GC.start
        o.should == sample_object
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

  it 'MessagePack.unpack symbolize_keys' do
    symbolized_hash = {:a => 'b', :c => 'd'}
    MessagePack.load(MessagePack.pack(symbolized_hash), :symbolize_keys => true).should == symbolized_hash
    MessagePack.unpack(MessagePack.pack(symbolized_hash), :symbolize_keys => true).should == symbolized_hash
  end

  it 'Unpacker#unpack symbolize_keys' do
    unpacker = MessagePack::Unpacker.new(:symbolize_keys => true)
    symbolized_hash = {:a => 'b', :c => 'd'}
    unpacker.feed(MessagePack.pack(symbolized_hash)).read.should == symbolized_hash
  end

  it "msgpack str 8 type" do
    MessagePack.unpack([0xd9, 0x00].pack('C*')).should == ""
    MessagePack.unpack([0xd9, 0x00].pack('C*')).encoding.should == Encoding::UTF_8
    MessagePack.unpack([0xd9, 0x01].pack('C*') + 'a').should == "a"
    MessagePack.unpack([0xd9, 0x02].pack('C*') + 'aa').should == "aa"
  end

  it "msgpack str 16 type" do
    MessagePack.unpack([0xda, 0x00, 0x00].pack('C*')).should == ""
    MessagePack.unpack([0xda, 0x00, 0x00].pack('C*')).encoding.should == Encoding::UTF_8
    MessagePack.unpack([0xda, 0x00, 0x01].pack('C*') + 'a').should == "a"
    MessagePack.unpack([0xda, 0x00, 0x02].pack('C*') + 'aa').should == "aa"
  end

  it "msgpack str 32 type" do
    MessagePack.unpack([0xdb, 0x00, 0x00, 0x00, 0x00].pack('C*')).should == ""
    MessagePack.unpack([0xdb, 0x00, 0x00, 0x00, 0x00].pack('C*')).encoding.should == Encoding::UTF_8
    MessagePack.unpack([0xdb, 0x00, 0x00, 0x00, 0x01].pack('C*') + 'a').should == "a"
    MessagePack.unpack([0xdb, 0x00, 0x00, 0x00, 0x02].pack('C*') + 'aa').should == "aa"
  end

  it "msgpack bin 8 type" do
    MessagePack.unpack([0xc4, 0x00].pack('C*')).should == ""
    MessagePack.unpack([0xc4, 0x00].pack('C*')).encoding.should == Encoding::ASCII_8BIT
    MessagePack.unpack([0xc4, 0x01].pack('C*') + 'a').should == "a"
    MessagePack.unpack([0xc4, 0x02].pack('C*') + 'aa').should == "aa"
  end

  it "msgpack bin 16 type" do
    MessagePack.unpack([0xc5, 0x00, 0x00].pack('C*')).should == ""
    MessagePack.unpack([0xc5, 0x00, 0x00].pack('C*')).encoding.should == Encoding::ASCII_8BIT
    MessagePack.unpack([0xc5, 0x00, 0x01].pack('C*') + 'a').should == "a"
    MessagePack.unpack([0xc5, 0x00, 0x02].pack('C*') + 'aa').should == "aa"
  end

  it "msgpack bin 32 type" do
    MessagePack.unpack([0xc6, 0x00, 0x00, 0x00, 0x00].pack('C*')).should == ""
    MessagePack.unpack([0xc6, 0x0, 0x00, 0x00, 0x000].pack('C*')).encoding.should == Encoding::ASCII_8BIT
    MessagePack.unpack([0xc6, 0x00, 0x00, 0x00, 0x01].pack('C*') + 'a').should == "a"
    MessagePack.unpack([0xc6, 0x00, 0x00, 0x00, 0x02].pack('C*') + 'aa').should == "aa"
  end

  class ValueOne
    attr_reader :num
    def initialize(num)
      @num = num
    end
    def ==(obj)
      self.num == obj.num
    end
    def num
      @num
    end
    def to_msgpack_ext
      @num.to_msgpack
    end
    def self.from_msgpack_ext(data)
      self.new(MessagePack.unpack(data))
    end
  end

  class ValueTwo
    attr_reader :num_s
    def initialize(num)
      @num_s = num.to_s
    end
    def ==(obj)
      self.num_s == obj.num_s
    end
    def num
      @num_s.to_i
    end
    def to_msgpack_ext
      @num_s.to_msgpack
    end
    def self.from_msgpack_ext(data)
      self.new(MessagePack.unpack(data))
    end
  end

  describe '#type_registered?' do
    it 'receive Class or Integer, and return bool' do
      expect(subject.type_registered?(0x00)).to be_falsy
      expect(subject.type_registered?(0x01)).to be_falsy
      expect(subject.type_registered?(::ValueOne)).to be_falsy
    end

    it 'returns true if specified type or class is already registered' do
      subject.register_type(0x30, ::ValueOne, :from_msgpack_ext)
      subject.register_type(0x31, ::ValueTwo, :from_msgpack_ext)

      expect(subject.type_registered?(0x00)).to be_falsy
      expect(subject.type_registered?(0x01)).to be_falsy

      expect(subject.type_registered?(0x30)).to be_truthy
      expect(subject.type_registered?(0x31)).to be_truthy
      expect(subject.type_registered?(::ValueOne)).to be_truthy
      expect(subject.type_registered?(::ValueTwo)).to be_truthy
    end

    it 'cannot detect unpack rule with block, not method' do
      subject.register_type(0x40){|data| ValueOne.from_msgpack_ext(data) }

      expect(subject.type_registered?(0x40)).to be_truthy
      expect(subject.type_registered?(ValueOne)).to be_falsy
    end
  end

  context 'with ext definitions' do
    it 'get type and class mapping for packing' do
      unpacker = MessagePack::Unpacker.new
      unpacker.register_type(0x01){|data| ValueOne.from_msgpack_ext }
      unpacker.register_type(0x02){|data| ValueTwo.from_msgpack_ext(data) }

      unpacker = MessagePack::Unpacker.new
      unpacker.register_type(0x01, ValueOne, :from_msgpack_ext)
      unpacker.register_type(0x02, ValueTwo, :from_msgpack_ext)
    end

    it 'returns a Array of Hash which contains :type, :class and :unpacker' do
      unpacker = MessagePack::Unpacker.new
      unpacker.register_type(0x02, ValueTwo, :from_msgpack_ext)
      unpacker.register_type(0x01, ValueOne, :from_msgpack_ext)

      list = unpacker.registered_types

      expect(list).to be_a(Array)
      expect(list.size).to eq(2)

      one = list[0]
      expect(one.keys.sort).to eq([:type, :class, :unpacker].sort)
      expect(one[:type]).to eq(0x01)
      expect(one[:class]).to eq(ValueOne)
      expect(one[:unpacker]).to eq(:from_msgpack_ext)

      two = list[1]
      expect(two.keys.sort).to eq([:type, :class, :unpacker].sort)
      expect(two[:type]).to eq(0x02)
      expect(two[:class]).to eq(ValueTwo)
      expect(two[:unpacker]).to eq(:from_msgpack_ext)
    end

    it 'returns a Array of Hash, which contains nil for class if block unpacker specified' do
      unpacker = MessagePack::Unpacker.new
      unpacker.register_type(0x01){|data| ValueOne.from_msgpack_ext }
      unpacker.register_type(0x02, &ValueTwo.method(:from_msgpack_ext))

      list = unpacker.registered_types

      expect(list).to be_a(Array)
      expect(list.size).to eq(2)

      one = list[0]
      expect(one.keys.sort).to eq([:type, :class, :unpacker].sort)
      expect(one[:type]).to eq(0x01)
      expect(one[:class]).to be_nil
      expect(one[:unpacker]).to be_instance_of(Proc)

      two = list[1]
      expect(two.keys.sort).to eq([:type, :class, :unpacker].sort)
      expect(two[:type]).to eq(0x02)
      expect(two[:class]).to be_nil
      expect(two[:unpacker]).to be_instance_of(Proc)
    end
  end

  def flatten(struct, results = [])
    case struct
    when Array
      struct.each { |v| flatten(v, results) }
    when Hash
      struct.each { |k, v| flatten(v, flatten(k, results)) }
    else
      results << struct
    end
    results
  end

  subject do
    described_class.new
  end

  let :buffer1 do
    MessagePack.pack(:foo => 'bar')
  end

  let :buffer2 do
    MessagePack.pack(:hello => {:world => [1, 2, 3]})
  end

  let :buffer3 do
    MessagePack.pack(:x => 'y')
  end

  describe '#execute/#execute_limit/#finished?' do
    let :buffer do
      buffer1 + buffer2 + buffer3
    end

    it 'extracts an object from the buffer' do
      subject.execute(buffer, 0)
      subject.data.should == {'foo' => 'bar'}
    end

    it 'extracts an object from the buffer, starting at an offset' do
      subject.execute(buffer, buffer1.length)
      subject.data.should == {'hello' => {'world' => [1, 2, 3]}}
    end

    it 'extracts an object from the buffer, starting at an offset reading bytes up to a limit' do
      subject.execute_limit(buffer, buffer1.length, buffer2.length)
      subject.data.should == {'hello' => {'world' => [1, 2, 3]}}
    end

    it 'extracts nothing if the limit cuts an object in half' do
      subject.execute_limit(buffer, buffer1.length, 3)
      subject.data.should be_nil
    end

    it 'returns the offset where the object ended' do
      subject.execute(buffer, 0).should == buffer1.length
      subject.execute(buffer, buffer1.length).should == buffer1.length + buffer2.length
    end

    it 'is finished if #data returns an object' do
      subject.execute_limit(buffer, buffer1.length, buffer2.length)
      subject.should be_finished

      subject.execute_limit(buffer, buffer1.length, 3)
      subject.should_not be_finished
    end
  end

  describe '#read' do
    context 'with a buffer' do
      it 'reads objects' do
        objects = []
        subject.feed(buffer1)
        subject.feed(buffer2)
        subject.feed(buffer3)
        objects << subject.read
        objects << subject.read
        objects << subject.read
        objects.should == [{'foo' => 'bar'}, {'hello' => {'world' => [1, 2, 3]}}, {'x' => 'y'}]
      end

      it 'reads map header' do
        subject.feed({}.to_msgpack)
        subject.read_map_header.should == 0
      end

      it 'reads array header' do
        subject.feed([].to_msgpack)
        subject.read_array_header.should == 0
      end
    end
  end

  describe '#each' do
    context 'with a buffer' do
      it 'yields each object in the buffer' do
        objects = []
        subject.feed(buffer1)
        subject.feed(buffer2)
        subject.feed(buffer3)
        subject.each do |obj|
          objects << obj
        end
        objects.should == [{'foo' => 'bar'}, {'hello' => {'world' => [1, 2, 3]}}, {'x' => 'y'}]
      end

      it 'returns an enumerator when no block is given' do
        subject.feed(buffer1)
        subject.feed(buffer2)
        subject.feed(buffer3)
        enum = subject.each
        enum.map { |obj| obj.keys.first }.should == %w[foo hello x]
      end
    end

    context 'with a StringIO stream' do
      it 'yields each object in the stream' do
        objects = []
        subject.stream = StringIO.new(buffer1 + buffer2 + buffer3)
        subject.each do |obj|
          objects << obj
        end
        objects.should == [{'foo' => 'bar'}, {'hello' => {'world' => [1, 2, 3]}}, {'x' => 'y'}]
      end
    end

    context 'with a File stream' do
      it 'yields each object in the stream' do
        objects = []
        file = Tempfile.new('msgpack')
        file.write(buffer1)
        file.write(buffer2)
        file.write(buffer3)
        file.open
        subject.stream = file
        subject.each do |obj|
          objects << obj
        end
        objects.should == [{'foo' => 'bar'}, {'hello' => {'world' => [1, 2, 3]}}, {'x' => 'y'}]
      end
    end

    context 'with a stream passed to the constructor' do
      it 'yields each object in the stream' do
        objects = []
        unpacker = described_class.new(StringIO.new(buffer1 + buffer2 + buffer3))
        unpacker.each do |obj|
          objects << obj
        end
        objects.should == [{'foo' => 'bar'}, {'hello' => {'world' => [1, 2, 3]}}, {'x' => 'y'}]
      end
    end
  end

  describe '#feed_each' do
    it 'feeds the buffer then runs #each' do
      objects = []
      subject.feed_each(buffer1 + buffer2 + buffer3) do |obj|
        objects << obj
      end
      objects.should == [{'foo' => 'bar'}, {'hello' => {'world' => [1, 2, 3]}}, {'x' => 'y'}]
    end

    it 'handles chunked data' do
      objects = []
      buffer = buffer1 + buffer2 + buffer3
      buffer.chars.each do |ch|
        subject.feed_each(ch) do |obj|
          objects << obj
        end
      end
      objects.should == [{'foo' => 'bar'}, {'hello' => {'world' => [1, 2, 3]}}, {'x' => 'y'}]
    end
  end

  describe '#fill' do
    it 'is a no-op' do
      subject.stream = StringIO.new(buffer1 + buffer2 + buffer3)
      subject.fill
      subject.each { |obj| }
    end
  end

  describe '#reset' do
    context 'with a buffer' do
      it 'is unclear what it is supposed to do'
    end

    context 'with a stream' do
      it 'is unclear what it is supposed to do'
    end
  end

  context 'regressions' do
    it 'handles massive arrays (issue #2)' do
      array = ['foo'] * 10_000
      MessagePack.unpack(MessagePack.pack(array)).should have(10_000).items
    end
  end

  context 'encoding', :encodings do
    before :all do
      @default_internal = Encoding.default_internal
      @default_external = Encoding.default_external
    end

    after :all do
      Encoding.default_internal = @default_internal
      Encoding.default_external = @default_external
    end

    let :buffer do
      MessagePack.pack({'hello' => 'world', 'nested' => ['object', {"sk\xC3\xA5l".force_encoding('utf-8') => true}]})
    end

    let :unpacker do
      described_class.new
    end

    before do
      Encoding.default_internal = Encoding::UTF_8
      Encoding.default_external = Encoding::ISO_8859_1
    end

    it 'produces results with default internal encoding' do
      unpacker.execute(buffer, 0)
      strings = flatten(unpacker.data).grep(String)
      strings.map(&:encoding).uniq.should == [Encoding.default_internal]
    end

    it 'recodes to internal encoding' do
      unpacker.execute(buffer, 0)
      unpacker.data['nested'][1].keys.should == ["sk\xC3\xA5l".force_encoding(Encoding.default_internal)]
    end
  end

  context 'extensions' do
    context 'symbolized keys' do
      let :buffer do
        MessagePack.pack({'hello' => 'world', 'nested' => ['object', {'structure' => true}]})
      end

      let :unpacker do
        described_class.new(:symbolize_keys => true)
      end

      it 'can symbolize keys when using #execute' do
        unpacker.execute(buffer, 0)
        unpacker.data.should == {:hello => 'world', :nested => ['object', {:structure => true}]}
      end

      it 'can symbolize keys when using #each' do
        objs = []
        unpacker.feed(buffer)
        unpacker.each do |obj|
          objs << obj
        end
        objs.should == [{:hello => 'world', :nested => ['object', {:structure => true}]}]
      end

      it 'can symbolize keys when using #feed_each' do
        objs = []
        unpacker.feed_each(buffer) do |obj|
          objs << obj
        end
        objs.should == [{:hello => 'world', :nested => ['object', {:structure => true}]}]
      end
    end

    context 'encoding', :encodings do
      let :buffer do
        MessagePack.pack({'hello' => 'world', 'nested' => ['object', {'structure' => true}]})
      end

      let :unpacker do
        described_class.new(:encoding => 'UTF-8')
      end

      it 'can hardcode encoding when using #execute' do
        unpacker.execute(buffer, 0)
        strings = flatten(unpacker.data).grep(String)
        strings.should == %w[hello world nested object structure]
        strings.map(&:encoding).uniq.should == [Encoding::UTF_8]
      end

      it 'can hardcode encoding when using #each' do
        objs = []
        unpacker.feed(buffer)
        unpacker.each do |obj|
          objs << obj
        end
        strings = flatten(objs).grep(String)
        strings.should == %w[hello world nested object structure]
        strings.map(&:encoding).uniq.should == [Encoding::UTF_8]
      end

      it 'can hardcode encoding when using #feed_each' do
        objs = []
        unpacker.feed_each(buffer) do |obj|
          objs << obj
        end
        strings = flatten(objs).grep(String)
        strings.should == %w[hello world nested object structure]
        strings.map(&:encoding).uniq.should == [Encoding::UTF_8]
      end
    end
  end
end
