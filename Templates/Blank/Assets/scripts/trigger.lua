-- Overlap events: this actor needs a CollisionComponent with physics enabled, the Pickup layer
-- and a mask that accepts Player (see scenes/trigger.scene.json). Move the Player with WASD.
-- It reads the reflected Target reference and sends the lamp a message.
properties = { cooldown = 1.5, sound = "audio/beep.wav", volume = 40 }

local ready = true

local function is_player(other)
	local player = world.player()
	return player ~= nil and other:id() == player:id()
end

function on_overlap_begin(self, other)
	if not ready or not is_player(other) then return end
	ready = false
	local lamp = self.actor:get("Target")
	if lamp then lamp:send("activate", { strength = 2, source = self.actor:name() }) end
	-- audio.play raises an error for a missing file; pcall keeps the script running.
	local ok, err = pcall(audio.play, self.props.sound, self.props.volume)
	if not ok then log("audio unavailable:", err) end
	timer.after(self.props.cooldown, function() ready = true end)
	log("trigger fired")
end

function on_overlap_end(self, other)
	if not is_player(other) then return end
	local lamp = self.actor:get("Target")
	if lamp then lamp:send("deactivate") end
end
