require 'mkmf'

have_header("ruby/st.h")
have_header("st.h")

$CFLAGS << %[ -I.. -Wall -O3 -g -std=c99]
#$CFLAGS << %[ -DDISABLE_STR_NEW_MOVE]
#$CFLAGS << %[ -DDISABLE_PREMEM]

if warnflags = CONFIG['warnflags']
  warnflags.slice!(/ -Wdeclaration-after-statement/)
end

create_makefile('msgpack/msgpack')

