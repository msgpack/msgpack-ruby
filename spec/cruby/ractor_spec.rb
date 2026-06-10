require 'spec_helper'

ractor_supported = defined?(Ractor) && RUBY_ENGINE == 'ruby'

describe 'Ractor safety', skip: (ractor_supported ? false : 'Ractor not supported on this Ruby') do
  def ractor_value(ractor)
    # Ractor#value replaced #take in newer rubies; support both.
    ractor.respond_to?(:value) ? ractor.value : ractor.take
  end

  # Ruby prints a one-time "Ractor API is experimental" warning to stderr. Quiet
  # it for this group, and restore afterwards so we don't suppress the warning
  # for unrelated specs sharing the process.
  before(:all) do
    @experimental_warning = Warning[:experimental]
    Warning[:experimental] = false
  end

  after(:all) do
    Warning[:experimental] = @experimental_warning
  end

  it 'round-trips via a Factory inside a non-main Ractor' do
    result = ractor_value(Ractor.new do
      factory = MessagePack::Factory.new
      factory.load(factory.dump([1, "two", 3.0, nil, true, {"k" => "v"}]))
    end)
    expect(result).to eq([1, "two", 3.0, nil, true, {"k" => "v"}])
  end

  it 'round-trips via a Packer and Unpacker inside a non-main Ractor' do
    result = ractor_value(Ractor.new do
      packed = MessagePack::Packer.new.write({"x" => [1, 2, 3]}).to_s
      MessagePack::Unpacker.new.feed(packed).read
    end)
    expect(result).to eq({"x" => [1, 2, 3]})
  end

  it 'packs and unpacks concurrently across many Ractors without corruption' do
    ractor_count = 8

    ractors = ractor_count.times.map do |n|
      Ractor.new(n) do |seed|
        obj = {
          "seed" => seed,
          "nums" => (0..20).to_a,
          "str" => "x" * 100,
          "nested" => {"deep" => [seed] * 10},
        }
        ok = true
        2_000.times do
          packed = MessagePack::Packer.new.write(obj).to_s
          ok &&= MessagePack::Unpacker.new.feed(packed).read == obj
        end
        ok
      end
    end

    expect(ractors.map { |r| ractor_value(r) }).to all(be true)
  end
end
