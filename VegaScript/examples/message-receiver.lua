function on_message(self, name, data)
	if name == "wave_started" then
		self.actor:set_rotation(data.count)
		log("Receiver: wave_started", data.count)
	end
end
