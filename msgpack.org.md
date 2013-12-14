# MessagePack for Ruby

## Example

    require 'msgpack'
    msg = [1,2,3].to_msgpack  #=> "\x93\x01\x02\x03"
    MessagePack.unpack(msg)   #=> [1,2,3]

## Install

    gem install msgpack

## Use cases

* Create REST API returing MessagePack using Rails + [RABL](https://github.com/nesquena/rabl)
* Store objects efficiently serialized by msgpack on memcached or Redis
* Upload data in efficient format from mobile devices such as smartphones
  * See also MessagePack for [Objective-C](https://github.com/msgpack/msgpack-objectivec) and [Java](https://github.com/msgpack/msgpack-java)
* Exchange objects with other components written in other languages

See [msgpack/msgpack-ruby on Github](https://github.com/msgpack/msgpack-ruby) for details.

