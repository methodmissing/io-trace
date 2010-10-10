class IO
  class Trace
    class Middleware
      def initialize(app, opts = {})
        @app = app
        @options = opts
      end

      def call(env)
        ret = nil
        IO.trace(@options[:strategy], @options[:stream], @options[:formatter]) do
          ret = @app.call(env)
        end
        ret
      end
    end
  end
end