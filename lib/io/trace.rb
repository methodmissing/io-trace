require File.expand_path('../../../ext/io/trace', __FILE__)

class IO
  class Trace
    BANNER = "File                                     Line    Syscall            FD   Feature\n\n".freeze

    autoload :Middleware, File.join(File.dirname(__FILE__), 'trace/middleware')

    # Initial formatters - work in progress
    #
    FORMATTERS = {
      :default => lambda{|t,s|
        s << BANNER
        s << t.aggregations.join
      },
      :time => lambda{|t,s|
        s << BANNER
        s << t.aggregations.select{|a| a.time? }.join
      },
      :calls => lambda{|t,s|
        s << BANNER
        s << t.aggregations.select{|a| a.calls? }.join
      },
      :cpu => lambda{|t,s|
        s << BANNER
        s << t.aggregations.select{|a| a.cpu? }.join
      },
      :bytes => lambda{|t,s|
        s << BANNER
        s << t.aggregations.select{|a| a.bytes? }.join
      },
      :fds => lambda{|t,s|
        s << "fds #{t.aggregations.map{|a| a.fd }.uniq.inspect}"
      },
      :files => lambda{|t,s|
        s << "files #{t.aggregations.map{|a| a.file }.uniq.inspect}"
      }
    }

    def self.formatter(name, &blk)
      raise ArgumentError.new("Expects a lambda / proc with 2 arguments") if blk.arity != 2
      FORMATTERS[name.to_sym] = blk
    end
  end

  # Main IO.trace interface
  #
  def self.trace(strategy = Trace::SUMMARY, stream = nil, formatter = nil, &b)
    r = Trace.new(trace_strategy(strategy))
    r.run(stream, formatter, &b)
  ensure
    r.close if r
  end

  private
  def self.trace_strategy(strategy)
    case strategy
    when Fixnum
      strategy
    when Symbol
      Trace.const_get(strategy)
    when NilClass
      Trace::SUMMARY
    else
      raise ArgumentError, "Unsupported trace strategy #{strategy.inspect}"
    end
  end
end