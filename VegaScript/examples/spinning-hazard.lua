properties = { interval = 0.05, step = 10 }

function on_start(self)
	timer.every(self.props.interval, function()
		self.actor:set_rotation(self.actor:rotation() + self.props.step)
	end)
end
