require 'spec_helper'
require 'json'

describe MessagePack do
  here = File.dirname(__FILE__)
  CASES         = File.read("#{here}/cases.msg")
  CASES_JSON    = File.read("#{here}/cases.json")
  CASES_COMPACT = File.read("#{here}/cases_compact.msg")

  it 'compare with json' do
    ms = []
    MessagePack::Unpacker.new.feed_each(CASES) do |m|
      ms << m
    end

    js = JSON.load(CASES_JSON)

    ms.zip(js) do |m, j|
      m.should == j
    end
  end

  it 'compare with compat' do
    ms = []
    MessagePack::Unpacker.new.feed_each(CASES) do |m|
      ms << m
    end

    cs = []
    MessagePack::Unpacker.new.feed_each(CASES_COMPACT) do |c|
      cs << c
    end

    ms.zip(cs) do |m, c|
      m.should == c
    end
  end
end
