begin
  require 'active_support/time_with_zone'

  ActiveSupport::TimeWithZone.class_eval do
    def to_msgpack(out='')
      self.to_s.to_msgpack(out)
    end
  end

rescue LoadError
  # Do nothing if ActiveSupport is not available.
end
