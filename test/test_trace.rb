class TestTrace < Test::Unit::TestCase
  FORMATTERS = [:default, :fds, :files, :time, :calls, :cpu, :bytes]
  def test_trace
    assert_match(/TestTrace/, IO.trace{ IO.read(__FILE__) })
    assert_raises ArgumentError do
      IO.trace(:undefined){}
    end
    assert_raises ArgumentError do
      IO.trace(/regex/){}
    end
  end

  def test_incompatible_formatter
    assert_raises ArgumentError do
      IO::Trace.formatter :test do |one_arg|
      end
    end
  end

  def test_formatter
    IO::Trace.formatter :test do |a,b|
    end
    assert IO::Trace::FORMATTERS.key?(:test)
    IO::Trace::FORMATTERS.delete(:test)
  end

  def test_trace_stream
    IO::Trace::STRATEGIES.each do |s|
      IO::Trace.formatters.each do |fmt|
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