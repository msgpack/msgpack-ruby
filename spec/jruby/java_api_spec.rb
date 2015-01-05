# encoding: ascii-8bit

require 'spec_helper'

# describe org.msgpack.jruby.RubyObjectPacker do
#   let :packer do
#     described_class.new(org.msgpack.MessagePack.new)
#   end

#   context '#packRaw' do
#     it 'returns a byte array' do
#       bytes = packer.packRaw(JRuby.runtime, false.to_java(org.jruby.RubyBoolean))
#       String.from_java_bytes(bytes).should == "\xc2"
#     end

#     it 'handles java null values' do
#       bytes = packer.packRaw(JRuby.runtime, nil)
#       String.from_java_bytes(bytes).should == "\xc0"
#     end

#     it 'supports an option hash' do
#       bytes = packer.packRaw(JRuby.runtime, false.to_java(org.jruby.RubyBoolean), {}.to_java(org.jruby.RubyHash))
#       String.from_java_bytes(bytes).should == "\xc2"
#     end

#     context 'deprecated' do
#       it 'returns a byte array' do
#         bytes = packer.packRaw(false.to_java(org.jruby.RubyBoolean))
#         String.from_java_bytes(bytes).should == "\xc2"
#       end

#       it 'does not support options' do
#         expect {
#           packer.packRaw(false.to_java(org.jruby.RubyBoolean), {}.to_java(org.jruby.RubyHash))
#         }.to raise_error
#       end

#       it 'handles java null values' do
#         bytes = packer.packRaw(nil)
#         String.from_java_bytes(bytes).should == "\xc0"
#       end
#     end
#   end
# end

# describe org.msgpack.jruby.RubyObjectUnpacker do
#   let :unpacker do
#     described_class.new(org.msgpack.MessagePack.new)
#   end

#   context '#unpack' do
#     it 'consumes a byte array' do
#       unpacker.unpack(JRuby.runtime, "\xc2".to_java_bytes)
#     end

#     it 'returns a ruby value' do
#       value = unpacker.unpack(JRuby.runtime, "\xc2".to_java_bytes)
#       value.should == false
#     end

#     it 'supports an option hash' do
#       unpacker.unpack(JRuby.runtime, "\xc2".to_java_bytes, {}.to_java(org.jruby.RubyHash))
#     end
#   end
# end
