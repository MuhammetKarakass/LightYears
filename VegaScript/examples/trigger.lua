local entered = false

function on_overlap_begin(self, other)
	local player = world.player()
	if entered or not player or other:id() ~= player:id() then return end
	entered = true
	for index = 1, 3 do
		world.spawn("DummyEnemy", 1400 + index * 100, 100)
	end
	local receiver = world.find("Receiver")
	if receiver then receiver:send("wave_started", { count = 3 }) end
	log("Wave spawned: 3 DummyEnemy")
end
