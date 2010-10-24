class Framework
  def self.generate(header)
    yield new(header) if block_given?
  end

  def initialize(header)
    @buf, @header, @ctx = [], header, nil
    File.delete(@header) if File.exist?(@header)
  end

  private
  def probes(ctx)
    @ctx = ctx ? ctx.to_s.downcase : 'summary'
    b RUBY if ctx
    self.class.const_get(ctx || :SUMMARY)
  end

  def save
    File.open(@header, File::CREAT | File::RDWR | File::APPEND) do |f|
      f << wrap("static char* #{@ctx}_script =")
      f << join_buffer
    end
  end

  def join_buffer
    @buf.join.split("\n").map{|l| "\"#{l.lstrip}\"\n" }.join.chomp << ';'
  ensure
    @buf.clear
  end

  def wrap(src); "\n#{src}\n"; end
  def b(d); @buf << d; end
end