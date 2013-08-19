require 'spec_helper'

describe MessagePack do
  it 'unpacks to the original data' do
    data = MessagePack::Extended.new(1, " " * (1 << 16))
    packed = MessagePack.pack(data)

    expect(MessagePack.unpack(packed))
      .to eql data
  end

  it "raises an error when type type is out of range" do
    expect { MessagePack::Extended.new(128, "") }
      .to raise_error

    expect { MessagePack::Extended.new(-129, "") }
      .to raise_error
  end

  describe ".eql?" do
    context "comparison with another object" do
      it "returns false" do
        a = MessagePack::Extended.new(1, "foo")
        b = "foo"

        expect(a).
          to_not eql b
      end
    end

    context "same type and data" do
      it "returns true" do
        a = MessagePack::Extended.new(1, "foo")
        b = MessagePack::Extended.new(1, "foo")

        expect(a).
          to eql b
      end
    end

    context "same type but different data" do
      it "returns false" do
        a = MessagePack::Extended.new(1, "foo")
        b = MessagePack::Extended.new(1, "bar")

        expect(a).
          to_not eql b
      end
    end

    context "different type but same data" do
      it "returns true" do
        a = MessagePack::Extended.new(1, "foo")
        b = MessagePack::Extended.new(2, "foo")

        expect(a).
          to_not eql b
      end
    end

  end

  describe "#create" do
    it "unpacks to the original data" do
      data = MessagePack::Extended.create(1, " " * (1 << 16))
      packed = MessagePack.pack(data)

      expect(MessagePack.unpack(packed))
        .to eql data
    end

    it "creates a MessagePack::Extended by default" do
      expect(MessagePack::Extended.create(1, " " * (1 << 16)))
        .to be_a MessagePack::Extended
    end
  end
end
