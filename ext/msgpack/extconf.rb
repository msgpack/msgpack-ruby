require 'mkmf'
require File.expand_path('../../lib/msgpack/version', File.dirname(__FILE__))
#$CFLAGS << %[ -I.. -Wall -O0 -DMESSAGEPACK_VERSION=\\"#{MessagePack::VERSION}\\" -g]
$CFLAGS << %[ -I.. -Wall -O3 -DMESSAGEPACK_VERSION=\\"#{MessagePack::VERSION}\\" -g]
create_makefile('msgpack')

