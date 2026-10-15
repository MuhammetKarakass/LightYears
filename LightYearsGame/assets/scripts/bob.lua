-- Capture the starting position on creation/reload and bob vertically around it.
properties = { amplitude = 30, period = 2 }
local origin
local elapsed = 0

function on_create(self)
	origin = self.actor:location()
end

function on_update(self, dt)
	elapsed = elapsed + dt
	local period = math.max(0.01, self.props.period)
	self.actor:set_location(origin.x, origin.y + math.sin(elapsed * 2 * math.pi / period) * self.props.amplitude)
end
