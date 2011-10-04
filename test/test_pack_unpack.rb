#!/usr/bin/env ruby
here = File.dirname(__FILE__)
require "#{here}/test_helper"

class MessagePackTestPackUnpack < Test::Unit::TestCase
	def test_nil
		check 1, nil
	end

	def test_true
		check 1, true
	end

	def test_false
		check 1, false
	end

	def test_zero
		check 1, 0
	end

	def test_positive_fixnum
		check 1, 1
		check 1, (1<<6)
		check 1, (1<<7)-1
	end

	def test_positive_int_8
		check 1, -1
		check 2, (1<<7)
		check 2, (1<<8)-1
	end

	def test_positive_int_16
		check 3, (1<<8)
		check 3, (1<<16)-1
	end

	def test_positive_int_32
		check 5, (1<<16)
		check 5, (1<<32)-1
	end

	def test_positive_int_64
		check 9, (1<<32)
		check 9, (1<<64)-1
	end

	def test_negative_fixnum
		check 1, -1
		check 1, -((1<<5)-1)
		check 1, -(1<<5)
	end

	def test_negative_int_8
		check 2, -((1<<5)+1)
		check 2, -(1<<7)
	end

	def test_negative_int_16
		check 3, -((1<<7)+1)
		check 3, -(1<<15)
	end

	def test_negative_int_32
		check 5, -((1<<15)+1)
		check 5, -(1<<31)
	end

	def test_negative_int_64
		check 9, -((1<<31)+1)
		check 9, -(1<<63)
	end

	def test_double
		check 9, 1.0
		check 9, 0.1
		check 9, -0.1
		check 9, -1.0
	end

	def test_fixraw
		check_raw 1, 0
		check_raw 1, (1<<5)-1
	end

	def test_raw_16
		check_raw 3, (1<<5)
		check_raw 3, (1<<16)-1
	end

	def test_raw_32
		check_raw 5, (1<<16)
		#check_raw 5, (1<<32)-1  # memory error
	end

	def test_fixarray
		check_array 1, 0
		check_array 1, (1<<4)-1
	end

	def test_array_16
		check_array 3, (1<<4)
		check_array 3, (1<<16)-1
	end

	def test_array_32
		check_array 5, (1<<16)
		#check_array 5, (1<<32)-1  # memory error
	end

  def test_emptyarray
    match [], "\x90"
  end

  def test_range_1
    obj = (0..14).to_a
    match obj, "\x9f\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e"
  end

  def test_range_2
    obj = (0..15).to_a
    match obj, "\xdc\x00\x10\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"
  end

	def test_nil
		match nil, "\xc0"
	end

	def test_false
		match false, "\xc2"
	end

	def test_true
		match true, "\xc3"
	end

	def test_0
		match 0, "\x00"
	end

	def test_127
		match 127, "\x7f"
	end

	def test_128
		match 128, "\xcc\x80"
	end

	def test_256
		match 256, "\xcd\x01\x00"
	end

	def test_negative_1
		match -1, "\xff"
	end

	def test_negative_33
		match -33, "\xd0\xdf"
	end

	def test_negative_129
		match -129, "\xd1\xff\x7f"
	end

	def test_map1_1
		obj = {1=>1}
		match obj, "\x81\x01\x01"
	end

	def test_1point0
		match 1.0, "\xcb\x3f\xf0\x00\x00\x00\x00\x00\x00"
	end

  def test_emptymap
		obj = {}
		match obj, "\x80"
	end

## FIXME
#	it "{0=>0, 1=>1, ..., 14=>14}" do
#		a = (0..14).to_a;
#		match Hash[*a.zip(a).flatten], "\x8f\x05\x05\x0b\x0b\x00\x00\x06\x06\x0c\x0c\x01\x01\x07\x07\x0d\x0d\x02\x02\x08\x08\x0e\x0e\x03\x03\x09\x09\x04\x04\x0a\x0a"
#	end
#
#	it "{0=>0, 1=>1, ..., 15=>15}" do
#		a = (0..15).to_a;
#		match Hash[*a.zip(a).flatten], "\xde\x00\x10\x05\x05\x0b\x0b\x00\x00\x06\x06\x0c\x0c\x01\x01\x07\x07\x0d\x0d\x02\x02\x08\x08\x0e\x0e\x03\x03\x09\x09\x0f\x0f\x04\x04\x0a\x0a"
#	end

## FIXME
#	it "fixmap" do
#		check_map 1, 0
#		check_map 1, (1<<4)-1
#	end
#
#	it "map 16" do
#		check_map 3, (1<<4)
#		check_map 3, (1<<16)-1
#	end
#
#	it "map 32" do
#		check_map 5, (1<<16)
#		#check_map 5, (1<<32)-1  # memory error
#	end

	def test_buffer
		str = "a"*32*1024*4
		raw = str.to_msgpack
		pac = MessagePack::Unpacker.new

		len = 0
		parsed = false

		n = 655
		time = raw.size / n
		time += 1 unless raw.size % n == 0
		off = 0

		time.times do
			assert(!parsed)

			fe = raw[off, n]
			assert(fe.length > 0)
			off += fe.length

			#pac.feed fe
			#pac.each {|obj|
			pac.feed_each(fe) {|obj|
				assert(!parsed)
				assert_equal(obj, str)
				parsed = true
			}
		end

		assert(parsed)
	end

	def test_gc_mark
		obj = [1024, {["a","b"]=>["c","d"]}, ["e","f"], "d", 70000, 4.12, 1.5, 1.5, 1.5]
		num = 4
		raw = obj.to_msgpack * num
		pac = MessagePack::Unpacker.new
		parsed = 0
		raw.split(//).each do |b|
			#pac.feed(b)
			pac.feed_each(b) {|o|
				GC.start
				assert_equal(obj, o)
				parsed += 1
			}
			GC.start
		end
		assert_equal(parsed, num)
	end

	def test_streaming_backward_compatibility
		obj = [1024, {["a","b"]=>["c","d"]}, ["e","f"], "d", 70000, 4.12, 1.5, 1.5, 1.5]
		num = 4
		raw = obj.to_msgpack * num
		pac = MessagePack::Unpacker.new
		buffer = ""
		nread  = 0
		parsed = 0
		raw.split(//).each do |b|
			buffer << b
			nread = pac.execute(buffer, nread)
			if pac.finished?
				o = pac.data
				assert_equal(obj, o)
				parsed += 1
				pac.reset
				buffer.slice!(0, nread)
				nread = 0
				next unless buffer.empty?
			end
		end
		assert_equal(parsed, num)
	end

#	def test_msgpack_version_constantk
#		p MessagePack::VERSION
#	end

	private
	def check(len, obj)
		v = obj.to_msgpack
		assert_equal(v.length, len)
		assert_equal(MessagePack.unpack(v), obj)
	end

	def check_raw(overhead, num)
		check num+overhead, " "*num
	end

	def check_array(overhead, num)
		check num+overhead, Array.new(num)
	end

	def match(obj, buf)
		assert_equal(obj.to_msgpack, buf)
		assert_equal(MessagePack::unpack(buf), obj)
	end
end

