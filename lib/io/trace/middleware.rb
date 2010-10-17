class IO
  class Trace
    class Middleware
      def initialize(app, opts = {})
        @app = app
        @options = opts
      end

      def call(env)
        IO.trace(@options[:strategy], @options[:stream], @options[:formatter]) do
          @app.call(env)
        end
      end
    end
  end
end