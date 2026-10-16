-- Receives "lap" messages from patrol_loop.lua and grows a little each time.
local laps = 0

function on_message(self, name, data)
	if name ~= "lap" then return end
	laps = laps + 1
	self.actor:set("Size", math.min(2, 0.5 + 0.1 * laps))
	log("laps completed:", data and data.laps or laps)
end
