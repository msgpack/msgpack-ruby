require 'spec_helper'

describe MessagePack do
  it 'unpacks to the original data' do
    data = MessagePack::ExtensionValue.new(1, " " * (1 << 16))
    packed = MessagePack.pack(data)

    expect(MessagePack.unpack(packed))
      .to eql data
  end

  it "raises an error when type type is out of range" do
    expect { MessagePack::ExtensionValue.new(128, "") }
      .to raise_error

    expect { MessagePack::ExtensionValue.new(-129, "") }
      .to raise_error
  end

  describe ".eql?" do
    context "comparison with another object" do
      it "returns false" do
        a = MessagePack::ExtensionValue.new(1, "foo")
        b = "foo"

        expect(a).
          to_not eql b
      end
    end

    context "same type and data" do
      it "returns true" do
        a = MessagePack::ExtensionValue.new(1, "foo")
        b = MessagePack::ExtensionValue.new(1, "foo")

        expect(a).
          to eql b
      end
    end

    context "same type but different data" do
      it "returns false" do
        a = MessagePack::ExtensionValue.new(1, "foo")
        b = MessagePack::ExtensionValue.new(1, "bar")

        expect(a).
          to_not eql b
      end
    end

    context "different type but same data" do
      it "returns true" do
        a = MessagePack::ExtensionValue.new(1, "foo")
        b = MessagePack::ExtensionValue.new(2, "foo")

        expect(a).
          to_not eql b
      end
    end

  end

  describe "#type=" do
    it "sets type" do
      ext = MessagePack::ExtensionValue.new(1, "foo")

      expect {
        ext.type = 2
      }.to change { ext.type }.from(1).to(2)
    end

    it "changed type is equal to initialized with same type" do
      initialized = MessagePack::ExtensionValue.new(2, "foo")
      changed     = MessagePack::ExtensionValue.new(1, "foo")

      changed.type = 2

      expect(initialized).to eql changed
    end
  end
end
