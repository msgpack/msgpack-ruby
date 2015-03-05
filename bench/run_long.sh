#!/bin/sh

# prerequisites
# $ sudo apt-get install sysstat
# $ rbenv shell 2.2.1 (or jruby-x.x.x or ...)
# $ rake install

echo "pack log long"
viiite report --regroup bench,threads bench/pack.rb &
sar -r 60 60 > pack_log_long.sar &

sleep 3700

echo "unpack log long"
viiite report --regroup bench,threads bench/unpack.rb &
sar -r 60 60 > unpack_log_long.sar & &

sleep 3700
