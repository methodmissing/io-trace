# Experimental middleware test / example
#

$:.unshift "."
require "lib/io/trace"
require "logger"
require "net/http"

app = lambda do |env|
  resp = Net::HTTP.get('www.wildfireapp.com', '/')
  [200, {"Content-Type" => "text/plain"}, [resp]]
end

use IO::Trace::Middleware, :strategy => :ALL, :stream => Logger.new(STDOUT)

run app