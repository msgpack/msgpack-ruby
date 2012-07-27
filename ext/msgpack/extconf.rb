require 'mkmf'
require File.expand_path('../../lib/msgpack/version', File.dirname(__FILE__))
have_header("ruby/st.h")
have_header("st.h")
$CFLAGS << %[ -I.. -Wall -O3 -DMESSAGEPACK_VERSION=\\"#{MessagePack::VERSION}\\" -g -std=c99]
#$CFLAGS << %[ -DDISABLE_STR_NEW_MOVE]
#$CFLAGS << %[ -DDISABLE_PREMEM]
CONFIG['warnflags'].slice!(/ -Wdeclaration-after-statement/)
create_makefile('msgpack/msgpack')

