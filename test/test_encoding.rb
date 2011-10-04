#!/usr/bin/env ruby
here = File.dirname(__FILE__)
require "#{here}/test_helper"

if RUBY_VERSION < "1.9"
	exit
end

class MessagePackTestEncoding < Test::Unit::TestCase
	def self.it(name, &block)
		define_method("test_#{name}", &block)
	end

  def test_us_ascii
    ckeck_unpack "abc".force_encoding("US-ASCII")
  end

  def test_utf_8_ascii
    check_unpack "abc".force_encoding("UTF-8")
  end

  def test_utf_8_mbstr
    check_unpack "\xE3\x81\x82".force_encoding("UTF-8")
  end

  def test_utf_8_invalid
    check_unpack "\xD0".force_encoding("UTF-8")
  end

  def test_ascii_8bit
    check_unpack "\xD0".force_encoding("ASCII-8BIT")
  end

  def test_euc_jp
    x = "\xA4\xA2".force_encoding("EUC-JP")
    check_unpack(x)
  end

  def test_euc_jp_invalid
    begin
      "\xD0".force_encoding("EUC-JP").to_msgpack
      assert(false)
    rescue Encoding::InvalidByteSequenceError
      assert(true)
    end
  end

	private
	def check_unpack(str)
		if str.encoding.to_s == "ASCII-8BIT"
			should_str = str.dup.force_encoding("UTF-8")
		else
			should_str = str.encode("UTF-8")
		end

		raw = str.to_msgpack
		r = MessagePack.unpack(str.to_msgpack)
		assert_equal(r.encoding.to_s, "UTF-8")
		assert_equal(r, should_str.force_encoding("UTF-8"))

		if str.valid_encoding?
			sym = str.to_sym
			r = MessagePack.unpack(sym.to_msgpack)
			assert_equal(r.encoding.to_s, "UTF-8")
			assert_equal(r, should_str.force_encoding("UTF-8"))
		end
	end
end
