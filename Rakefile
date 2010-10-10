#!/usr/bin/env rake
require 'rake/testtask'
require 'rake/clean'
$:.unshift(File.expand_path('lib'))
TRACE_ROOT = 'ext/io'

desc 'Default: test'
task :default => :test

desc 'Run trace tests.'
Rake::TestTask.new(:test) do |t|
  t.libs = [TRACE_ROOT]
  t.pattern = 'test/test_*.rb'
  t.ruby_opts << '-rtest'
  t.libs << 'test'
  t.warning = true
  t.verbose = true
end
task :test => :build

namespace :build do
  file "#{TRACE_ROOT}/probes.h"
  file "#{TRACE_ROOT}/trace.h"
  file "#{TRACE_ROOT}/trace.d"
  file "#{TRACE_ROOT}/trace.c"
  file "#{TRACE_ROOT}/extconf.rb"
  file "#{TRACE_ROOT}/Makefile" => %W(#{TRACE_ROOT}/trace.c #{TRACE_ROOT}/trace.h #{TRACE_ROOT}/extconf.rb) do
    Dir.chdir(TRACE_ROOT) do
      ruby 'extconf.rb'
    end
  end

  desc "generate makefile"
  task :makefile => %W(#{TRACE_ROOT}/Makefile #{TRACE_ROOT}/trace.c #{TRACE_ROOT}/trace.h)

  dlext = Config::CONFIG['DLEXT']
  file "#{TRACE_ROOT}/trace.#{dlext}" => %W(#{TRACE_ROOT}/Makefile #{TRACE_ROOT}/trace.c #{TRACE_ROOT}/trace.h) do
    Dir.chdir(TRACE_ROOT) do
      sh 'make' # TODO - is there a config for which make somewhere?
    end
  end

  desc "compile trace extension"
  task :compile => "#{TRACE_ROOT}/trace.#{dlext}"

  task :clean do
    Dir.chdir(TRACE_ROOT) do
      sh 'make clean'
    end if File.exists?("#{TRACE_ROOT}/Makefile")
  end

  CLEAN.include("#{TRACE_ROOT}/Makefile")
  CLEAN.include("#{TRACE_ROOT}/trace.#{dlext}")
end

task :clean => %w(build:clean)

desc "compile"
task :build => %w(build:compile)

task :install do |t|
  Dir.chdir(TRACE_ROOT) do
    sh 'sudo make install'
  end
end

desc "clean build install"
task :setup => %w(clean build install)

namespace :test do
  desc 'middleware'
  task :middleware do
    sh 'sudo rackup'
  end
end
task 'test:middleware' => :build