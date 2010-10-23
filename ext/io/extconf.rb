require 'mkmf'

# Flaky extconf.rb - OS X Leopard centric atm. with no special handling for BSD, Solaris or
# environments with a broken dtrace setup

dir_config('trace')
$defs.push("-pedantic")

case RUBY_PLATFORM
  when /solaris/, /bsd/, /darwin/
    find_library('dtrace', 'dtrace_open')
    system("dtrace -h -o probes.h -s dtrace.d")
    require File.expand_path('../frameworks/dtrace', __FILE__)
  when /linux/
    raise "SystemTap support pending!"
  else
    raise "Platform #{RUBY_PLATFORM} not supported!"
end

create_makefile('trace')