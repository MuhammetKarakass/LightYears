-- Messages and spawning: reacts to "activate" / "deactivate" sent by trigger.lua.
-- Data is copied; tables need string keys. world.spawn takes a reflected type or a prefab path.
properties = { spark_prefab = "prefabs/Spark.prefab.json", sparks = 3, spark_lifetime = 1.5, radius = 90 }

local base_size = 1
local live_sparks = 0

function on_start(self)
	base_size = self.actor:get("Size") or 1
end

local function spawn_sparks(self)
	local origin = self.actor:location()
	local count = self.props.sparks
	for i = 1, count do
		if live_sparks >= 12 then return end
		local angle = (i / count) * 2 * math.pi
		local x = origin.x + math.cos(angle) * self.props.radius
		local y = origin.y + math.sin(angle) * self.props.radius
		-- The props table sets reflected actor properties, here Rotation.
		local spark = world.spawn(self.props.spark_prefab, x, y, { Rotation = i * (360 / count) })
		if spark then
			live_sparks = live_sparks + 1
			timer.after(self.props.spark_lifetime, function()
				spark:destroy()
				live_sparks = live_sparks - 1
			end)
		end
	end
end

function on_message(self, name, data)
	if name == "activate" then
		local strength = data and data.strength or 2
		self.actor:set("Size", base_size * strength)
		spawn_sparks(self)
		log("lamp activated by", data and data.source or "unknown")
	elseif name == "deactivate" then
		self.actor:set("Size", base_size)
	end
end
