require 'mkmf'
require File.expand_path(File.dirname(__FILE__) + '../../../lib/msgpack/version')
$CFLAGS << %[ -I.. -Wall -O3 -DMESSAGEPACK_VERSION=\\"#{MessagePack::VERSION}\\" -g]
create_makefile('msgpack')

