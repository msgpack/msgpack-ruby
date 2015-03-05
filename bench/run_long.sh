#!/bin/sh

# prerequisites
# $ sudo apt-get install sysstat
# $ rbenv shell 2.2.1 (or jruby-x.x.x or ...)
# $ rake install

echo "pack log long"
viiite report --regroup bench,threads bench/pack_log_long.rb &
sar -r 60 600 > pack_log_long.sar &

sleep 36060 # 60*60 * 5[threads] * 2[bench] + 60 (cool down)

echo "unpack log long"
viiite report --regroup bench,threads bench/unpack_log_long.rb &
sar -r 60 600 > unpack_log_long.sar & &

sleep 36060 # 60*60 * 5[threads] * 2[bench] + 60 (cool down)

