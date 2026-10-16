-- Hierarchy: rotating a parent moves every attached child with it.
-- After a few seconds the first child is detached, then attached again.
properties = { degrees_per_second = 45, detach_after = 5, attach_after = 8 }

function on_start(self)
	local children = self.actor:children()
	for index, child in ipairs(children) do
		log("child", index, child:name(), "of", self.actor:name())
	end
	local first = children[1]
	if not first then return end
	timer.after(self.props.detach_after, function()
		first:detach()
		log("detached", first:name(), "parent alive:", first:parent() ~= nil)
	end)
	timer.after(self.props.attach_after, function()
		-- The optional second argument keeps world values (default true).
		log("re-attached", first:name(), first:attach_to(self.actor))
	end)
end

function on_update(self, dt)
	self.actor:set_rotation(self.actor:rotation() + self.props.degrees_per_second * dt)
end
