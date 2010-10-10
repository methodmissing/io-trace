spec = Gem::Specification.new do |s|
  s.name = 'io-trace'
  s.version = '0.0.1'
  s.date = '2010-10-10'
  s.summary = 'IO tracing framework for Ruby MRI'
  s.description = "IO tracing framework for Ruby MRI"
  s.homepage = "http://github.com/methodmissing/io-trace"
  s.has_rdoc = false
  s.authors = ["Lourens Naudé"]
  s.email = ["lourens@methodmissing.com"]
  s.extensions = "ext/io/extconf.rb"
  s.files = `git ls-files`.split
end