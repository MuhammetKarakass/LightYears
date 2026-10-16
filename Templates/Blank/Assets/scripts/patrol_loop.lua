-- Vec2 list: walks the reflected "Patrol" points in a loop. Vectors support +, -, * and :length().
-- After each lap it messages the actor stored in the reflected "Target" reference.
properties = { speed = 120 }

local points = {}
local index = 1
local laps = 0

function on_start(self)
	-- Reads allocate a table; cache it once. The table has a field n with its length.
	points = self.actor:get("Patrol")
end

function on_update(self, dt)
	local count = points.n or 0
	if count == 0 then return end
	local position = self.actor:location()
	local delta = points[index] - position
	local step = self.props.speed * dt
	if delta:length() <= step then
		self.actor:set_location(points[index].x, points[index].y)
		index = index % count + 1
		if index == 1 then
			laps = laps + 1
			local counter = self.actor:get("Target")
			if counter then counter:send("lap", { laps = laps }) end
		end
	else
		local move = delta:normalize() * step
		self.actor:set_location(position.x + move.x, position.y + move.y)
	end
end
