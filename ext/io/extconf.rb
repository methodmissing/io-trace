require 'mkmf'

require File.expand_path('../frameworks/dtrace', __FILE__)
Dtrace.generate('scripts.h') do |h|
  h.strategy :SUMMARY
  h.strategy :READ
  h.strategy :WRITE
  h.strategy :SETUP
  h.strategy :ALL
end

# Flaky extconf.rb - OS X Leopard centric atm. with no special handling for BSD, Solaris or
# environments with a broken dtrace setup

dir_config('trace')
$defs.push("-pedantic")
find_library('dtrace', 'dtrace_open')
system("dtrace -h -o probes.h -s trace.d")
create_makefile('trace')