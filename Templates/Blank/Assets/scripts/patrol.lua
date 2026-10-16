-- Authored world-space points; the last point opens the connected target.
local index = 1
local points, target
function on_start(self)
	points = self.actor:get("Patrol")
	target = self.actor:get("Target")
end
function on_update(self, dt)
	local point = points[index]
	if not point then return end
	local position = self.actor:location()
	local delta = point - position
	local distance = delta:length()
	if distance <= 240 * dt then
		self.actor:set_location(point.x, point.y)
		index = index + 1
		if index > #points then
			if target then target:set("Size", 0.5) end
		end
	else
		local step = delta:normalize() * (240 * dt)
		self.actor:set_location(position.x + step.x, position.y + step.y)
	end
end
