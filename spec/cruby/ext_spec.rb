# encoding: ascii-8bit
require 'spec_helper'

describe ExtensionValue do
  it 'accessors' do
    ext = ExtensionValue.new(1, "data")
    ext.type.should == 1
    ext.payload.should == "data"
  end

  it 'equals' do
    ext1 = ExtensionValue.new(1, "data")
    ext2 = ExtensionValue.new(1, "data")
    ext3 = ExtensionValue.new(2, "data")
    ext4 = ExtensionValue.new(1, "value")
    (ext1 == ext2).should be_true
    (ext1 == ext3).should be_false
    (ext1 == ext4).should be_false
    ext1.eql?(ext2).should be_true
    ext1.eql?(ext3).should be_false
    ext1.eql?(ext4).should be_false
    (ext1.hash == ext2.hash).should be_true
    (ext1.hash == ext3.hash).should be_false
    (ext1.hash == ext4.hash).should be_false
  end
end
