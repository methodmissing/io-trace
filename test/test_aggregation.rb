class TestAggregation < Test::Unit::TestCase
  def test_aggregation
    a = aggregation :syscall => 'read',
                    :metric => 'cpu',
                    :fd => 4,
                    :file => 'aggregation.rb',
                    :line => 12,
                    :value => 2000
    assert_equal 'read', a.syscall
    assert_equal 'cpu', a.metric
    assert_equal 4, a.fd
    assert_equal 'aggregation.rb', a.file
    assert_equal 12, a.line
    assert_equal 0.002, a.value
    assert_equal "aggregation.rb                           12      read               4    cpu        0.002 ms\n", a.inspect
    assert a.cpu?
  end

  private
  def aggregation(values = {})
    IO::Trace::Aggregation.new(values)
  end
end