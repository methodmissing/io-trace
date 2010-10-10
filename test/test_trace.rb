class TestTrace < Test::Unit::TestCase
  STRATEGIES = [:SUMMARY, :READ, :WRITE, :SETUP, :ALL]
  FORMATTERS = [:default, :fds, :files, :time, :calls, :cpu, :bytes]
  def test_trace
    IO.trace{ IO.read(__FILE__) }
  end

  def test_trace_stream
    STRATEGIES.each do |s|
      FORMATTERS.each do |fmt|
        logger = Logger.new(STDOUT)
        puts
        pp "======================================"
        pp " strategy #{s.inspect}, formatter #{fmt.inspect}"
        pp "======================================"
        IO.trace(s, logger, fmt){
          IO.read(__FILE__)
          puts
          require 'uri'
        }
      end
    end
  end
end