require 'mkmf'

# Flaky extconf.rb - OS X Leopard centric atm. with no special handling for BSD, Solaris or
# environments with a broken dtrace setup

dir_config('trace')
$defs.push("-pedantic")

def add_define(name)
  $defs.push("-D#{name}")
end

require File.expand_path('../frameworks/framework', __FILE__)

case RUBY_PLATFORM
  when /solaris/, /bsd/, /darwin/
    add_define('HAVE_DTRACE') if find_library('dtrace', 'dtrace_open')
    system("dtrace -h -o probes.h -s trace.d")
    require File.expand_path('../frameworks/dtrace', __FILE__)
  when /linux/
    add_define('HAVE_SYSTEMTAP') if have_header('sys/sdt.h')
    system("dtrace -h -o probes.h -s trace.d")
    require File.expand_path('../frameworks/systemtap', __FILE__)
  else
    raise "Platform #{RUBY_PLATFORM} not supported!"
end

create_makefile('trace')