-- Timers: timer.every repeats, timer.after fires once, both return a handle that can be cancelled.
-- Timers belong to this script instance and are cancelled automatically on reload or destroy.
properties = { pulse = 0.4, amount = 0.15, stop_after = 4 }

local base_size = 1
local grow = true
local pulser

function on_start(self)
	base_size = self.actor:get("Size") or 1
	pulser = timer.every(self.props.pulse, function()
		grow = not grow
		self.actor:set("Size", base_size * (grow and 1 + self.props.amount or 1))
	end)
	-- A one-shot timer cancels the repeating one, then restores the authored size.
	local stopper = timer.after(self.props.stop_after, function()
		pulser:cancel()
		self.actor:set("Size", base_size)
		log("pulse stopped, active:", pulser:is_active())
	end)
	log("timers started, stopper active:", stopper:is_active())
end
