-- Rotate the owning actor at an editable speed in degrees per second.
properties = { speed = 120 }

function on_update(self, dt)
	self.actor:set_rotation(self.actor:rotation() + self.props.speed * dt)
end
