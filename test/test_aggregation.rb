class TestAggregation < Test::Unit::TestCase
  def test_aggregation
    a = aggregation :probe => 'read',
                    :feature => 'cpu',
                    :fd => 4,
                    :file => 'aggregation.rb',
                    :line => 12,
                    :value => 200
    assert_equal 'read', a.probe 
    assert_equal 'cpu', a.feature
    assert_equal 4, a.fd
    assert_equal 'aggregation.rb', a.file
    assert_equal 12, a.line
    assert_equal 0.0002, a.value
    assert_equal "aggregation.rb                           12      read               4    cpu        0.000200 ms\n", a.inspect
    assert a.cpu?
  end

  private
  def aggregation(values = {})
    IO::Trace::Aggregation.new(values)
  end
end