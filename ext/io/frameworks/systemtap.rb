class Systemtap < Framework
  def strategy(ctx = nil)
    ctx = nil if ctx == :SUMMARY
    @ctx = ctx ? ctx.to_s.downcase : 'summary'
    b 'test'
    save
  end
end

Systemtap.generate('scripts.h') do |h|
  h.strategy :SUMMARY
  h.strategy :READ
  h.strategy :WRITE
  h.strategy :SETUP
  h.strategy :ALL
end