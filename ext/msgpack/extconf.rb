require 'mkmf'

have_header("ruby/st.h")
have_header("st.h")
have_func("rb_str_replace", ["ruby.h"])

$CFLAGS << %[ -I.. -Wall -O3 -g -std=c99]
#$CFLAGS << %[ -DDISABLE_RMEM]
#$CFLAGS << %[ -DDISABLE_RMEM_REUSE_INTERNAL_FRAGMENT]
#$CFLAGS << %[ -DDISABLE_BUFFER_READ_REFERENCE_OPTIMIZE]
#$CFLAGS << %[ -DDISABLE_BUFFER_READ_TO_S_OPTIMIZE]

if warnflags = CONFIG['warnflags']
  warnflags.slice!(/ -Wdeclaration-after-statement/)
end

create_makefile('msgpack/msgpack')

