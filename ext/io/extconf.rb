require 'mkmf'

# Flaky extconf.rb - OS X Leopard centric atm. with no special handling for BSD, Solaris or
# environments with a broken dtrace setup

dir_config('trace')
$defs.push("-pedantic")
find_library('dtrace', 'dtrace_open')
system("dtrace -h -o probes.h -s trace.d")
create_makefile('trace')