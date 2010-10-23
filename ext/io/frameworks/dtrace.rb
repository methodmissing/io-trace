class Dtrace
  READ = %w(read pread readv recv)
  WRITE = %w(write pwrite writev send)
  SETUP = %w(bind socketpair getpeername shutdown open stat lstat fstat select poll connect fsync accept fcntl)
  SUMMARY = READ + WRITE + SETUP
  ALL = READ + WRITE + SETUP

  FDS = READ + WRITE + %w(close connect fsync accept fcntl)
  BYTES = READ + WRITE

  RUBY = %[ruby*:::line
           /pid == $pid/
           {
             self->file = copyinstr(arg0);
             self->line = arg1;
           }\n]

  ENTRY_PREDICATE = "/self->line/"
  RETURN_PREDICATE = "/self->line && self->ts/"

  BODY_ENTRY = %[self->ts = timestamp;
                 self->vts = vtimestamp;
                 self->fd = %s;
                 %s
                 @calls[probefunc,self->file,self->line,self->fd] = count();]

  BODY_RETURN = %[this->elapsed = timestamp - self->ts;
                  this->cpu = vtimestamp - self->vts;
                  @cpu[probefunc,self->file,self->line,self->fd] = sum(this->cpu);
                  @time[probefunc,self->file,self->line,self->fd] = sum(this->elapsed);
                  %s
                  this->elapsed = 0;
                  this->cpu = 0;
                  self->ts = 0;
                  self->vts = 0;
                  self->line = 0;
                  self->file = 0;]

  def self.generate(header)
    yield new(header) if block_given?
  end

  def initialize(header)
    @buf, @header, @ctx = [], header, nil
    File.delete(@header) if File.exist?(@header)
  end

  def strategy(ctx = nil)
    ctx = nil if ctx == :SUMMARY
    prbs = probes(ctx)
    fds = prbs & FDS
    entries(fds, ctx, 'arg0')
    entries(prbs - fds, ctx, '-1')
    bytes = prbs & BYTES
    returns(bytes, '@bytes[probefunc,self->file,self->line,self->fd] = sum(arg0);')
    returns(prbs - bytes)
    save
  end

  private
  def probes(ctx)
    @ctx = ctx ? ctx.to_s.downcase : 'summary'
    b RUBY if ctx
    self.class.const_get(ctx || :SUMMARY)
  end

  def entries(probes, ctx, fd)
    return if probes.empty?
    data = ctx ? [fd, ''] : [-1, 'self->file = \"(all)\";self->line = -1;']
    list 'entry', probes
    b ctx ? wrap(ENTRY_PREDICATE) : wrap("/pid == $pid/")
    b "{\n#{BODY_ENTRY}\n}\n" % data
  end

  def returns(probes, data = '')
    return if probes.empty?
    list 'return', probes
    b wrap(RETURN_PREDICATE)
    b "{\n#{BODY_RETURN}\n}\n" % data
  end

  def list(scope, probes)
    b probes.map{|p| "syscall::#{p}*:#{scope}" }.join(",\n")
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

Dtrace.generate('scripts.h') do |h|
  h.strategy :SUMMARY
  h.strategy :READ
  h.strategy :WRITE
  h.strategy :SETUP
  h.strategy :ALL
end