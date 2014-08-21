require 'spec_helper'
require 'active_support/core_ext/numeric/time'

describe ActiveSupport::TimeWithZone do
  it 'should correctly pack a TimeWithZone object, including timezone' do
    Time.zone = 'Pacific Time (US & Canada)'
    initial_time = Time.zone.local(2014, 8, 21, 15, 30, 0)

    packed_time = MessagePack.pack(initial_time)
    packed_time.should == "\xB92014-08-21 15:30:00 -0700"

    unpacked_time = MessagePack.unpack(packed_time)
    unpacked_time.should == "2014-08-21 15:30:00 -0700"

    parsed_time = Time.zone.parse(unpacked_time)
    parsed_time.should == initial_time
  end
end
