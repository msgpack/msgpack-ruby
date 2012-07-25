require 'mkmf'
require File.expand_path('../../lib/msgpack/version', File.dirname(__FILE__))
have_header("ruby/st.h")
have_header("st.h")
$CFLAGS << %[ -I.. -Wall -O3 -DMESSAGEPACK_VERSION=\\"#{MessagePack::VERSION}\\" -g]
#$CFLAGS << %[ -DDISABLE_STR_NEW_MOVE]
#$CFLAGS << %[ -DDISABLE_PREMEM]
create_makefile('msgpack/msgpack')

