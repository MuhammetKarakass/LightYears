-- A bounded quarter turn makes Play/Stop and automated checks easy to observe.
properties = { speed = 360 }

function on_update(self, dt)
	self.actor:set_rotation(math.min(90, self.actor:rotation() + self.props.speed * dt))
end
