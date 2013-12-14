# MessagePack for Ruby

```ruby
require 'msgpack'
msg = [1,2,3].to_msgpack  #=> "\x93\x01\x02\x03"
MessagePack.unpack(msg)   #=> [1,2,3]
```

## Install

```ruby
gem install msgpack
```

## Use cases

* Create REST API returing MessagePack using Rails + [RABL](https://github.com/nesquena/rabl)
* Store objects efficiently in memcached or Redis
* Upload data in efficient format from mobile devices. See also MessagePack for [Objective-C](https://github.com/msgpack/msgpack-objectivec) and [Java](https://github.com/msgpack/msgpack-java)

## Streaming API

```ruby
# serialize a 2-element array [e1, e2]
pk = MessagePack::Packer.new(io)
pk.write_array_header(2).write(e1).write(e2).flush
```

```ruby
# deserialize objects from an IO
u = MessagePack::Unpacker.new(io)
u.each { |obj| ... }
```

```ruby
# event-driven deserialization
def on_read(data)
  @u ||= MessagePack::Unpacker.new
  @u.feed_each(data) { |obj| ... }
end
```

## Links

* [Github](https://github.com/msgpack/msgpack-ruby)
* [API document](http://ruby.msgpack.org/)

