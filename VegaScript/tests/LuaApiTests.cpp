#include "framework/EngineContext.h"
#include <script/ScriptComponent.h>
#include <script/ScriptRuntime.h>
#include <framework/Actor.h>
#include <framework/World.h>
#include <framework/RenderViewport.h>
#include <framework/AudioManager.h>
#include <framework/scene/Scene.h>
#include <framework/scene/SceneEntityComponent.h>
#include <framework/scene/PrefabLibrary.h>
#include <framework/debug/LogBuffer.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <thread>

namespace
{
	using vega::script::ScriptComponent;
	using vega::script::ScriptRuntime;
	std::filesystem::path root;
	int failures = 0;
	void Check(bool ok, const std::string& message)
	{
		if (!ok) { ++failures; std::cerr << message << '\n'; }
	}
	void Write(const std::string& path, const std::string& text)
	{
		std::filesystem::create_directories((root / path).parent_path());
		std::ofstream{ root / path, std::ios::binary | std::ios::trunc } << text;
	}
	ScriptComponent& Attach(ly::Actor& actor, const std::string& name, const std::string& text)
	{
		Write(name + ".lua", text);
		return actor.AddComponent<ScriptComponent>(name + ".lua");
	}
	void Healthy(const ScriptComponent& script) { Check(script.LastError().empty(), script.ScriptPath() + ": " + script.LastError()); }
	class LuaActor : public ly::Actor
	{
	public:
		using ly::Actor::Actor;
	};
	std::shared_ptr<ly::Actor> Named(ly::World& world, const std::string& name)
	{
		auto actor = world.SpawnActor<LuaActor>().lock();
		actor->AddComponent<ly::SceneEntityComponent>("", name);
		return actor;
	}
	class TestBody : public ly::Actor
	{
	public:
		explicit TestBody(ly::World* world) : Actor{ world }
		{
			SetCollisionLayer(CollisionLayer::Layer0);
			SetCollisionMask(CollisionLayer::Layer0);
			SetEnablePhysics(true);
		}
		float GetPhysicsCollisionRadius() const override { return 20.f; }
		// Deliberately does not forward to Actor: component events must still arrive.
		void OnActorBeginOverlap(ly::Actor*) override {}
	};
	class InputViewport : public ly::RenderViewport
	{
	public:
		bool focused = true;
		sf::RenderTarget& GetRenderTarget() override { throw std::runtime_error("Unused test target"); }
		const sf::RenderTarget& GetRenderTarget() const override { throw std::runtime_error("Unused test target"); }
		sf::Vector2u GetSize() const override { return { 800, 600 }; }
		sf::Vector2i GetMousePixelPosition() const override { return { 7, 11 }; }
		sf::Vector2f MapPixelToCoords(sf::Vector2i pixel, const sf::View& view) const override
		{
			return sf::Vector2f{ static_cast<float>(pixel.x), static_cast<float>(pixel.y) } + view.getCenter();
		}
		sf::View GetDefaultView() const override { return sf::View{ { 0, 0 }, { 800, 600 } }; }
		bool HasInputFocus() const override { return focused; }
	};
	void Hierarchy()
	{
		ly::World world{ nullptr };
		auto owner = Named(world, "Director");
		Write("prefabs/tree.prefab.json", R"({"format":"vega.prefab","version":1,"base":{"type":"LuaActor"},"children":[{"id":"branch","name":"Branch","type":"LuaActor","properties":{"Location":[10,0]}}]})");
		world.TickInternal(0.f);
		auto& script = Attach(*owner, "hierarchy", R"(
function on_start(self)
 local root = world.spawn('prefabs/tree.prefab.json',100,200)
 assert(#root:children()==1)
 local child=root:children()[1]
 assert(child:parent():id()==root:id())
 assert(child:location().x==110 and child:location().y==200)
 root:set_rotation(90)
 assert(math.abs(child:location().x-100)<0.001 and math.abs(child:location().y-210)<0.001)
 child:set_location(100,220)
 child:detach()
 assert(child:parent()==nil and #root:children()==0)
 assert(child:attach_to(root) and #root:children()==1)
 assert(not root:attach_to(child))
 local foreign=world.spawn('LuaActor',0,0)
 assert(child:attach_to(foreign,false))
 assert(math.abs(child:location().x-20)<0.001)
 assert(child:attach_to(root))
 root:destroy()
 assert(not child:is_alive() and child:parent()==nil and #root:children()==0)
 foreign:destroy()
 self.actor:set_rotation(123)
end
)");
		Healthy(script);
		Check(owner->GetActorRotation() == 123.f, "Lua hierarchy assertions did not finish");
	}
	void WorldApi()
	{
		ly::World world{ nullptr }, foreign{ nullptr };
		auto owner = Named(world, "Director");
		auto near = Named(world, "Near");
		near->SetActorLocation({ 3, 4 });
		auto far = Named(world, "Far");
		far->SetActorLocation({ 500, 500 });
		ScriptRuntime::Get().SetPlayerResolver([near](ly::World&) { return near; });
		Write("prefabs/test.prefab.json", R"({"format":"vega.prefab","version":1,"name":"Test","base":{"type":"LuaActor"},"properties":{"Rotation":17}})");
		world.TickInternal(0.f);
		auto& script = Attach(*owner, "world", R"(
return { on_start = function(self)
 assert(io == nil and os == nil and require == nil and debug == nil and load == nil)
 assert(world.find('missing') == nil and world.find('Near'):name() == 'Near')
 assert(#world.find_all_by_type('Actor') == 3 and #world.find_all_by_type('unknown') == 0)
 assert(#world.find_in_radius(0,0,5) == 2 and #world.find_in_radius(0,0,5,'Actor') == 2)
 assert(#world.find_in_radius(0,0,5,'Missing') == 0)
 assert(world.player():id() == world.find('Near'):id())
 assert(self.actor:distance_to(world.find('Near')) == 5)
 self.actor:look_at(10,0)
 assert(math.abs(self.actor:forward().x - 1) < 0.0001)
 local a = world.spawn('LuaActor',10,20,{Rotation=25,RenderLayer=2})
 assert(a:is_alive() and a:location().x == 10 and a:rotation() == 25 and a:get('RenderLayer') == 2)
 local b = world.spawn('prefabs/test.prefab.json',30,40)
 assert(b:is_alive() and b:location().y == 40 and b:rotation() == 17)
 assert(#world.find_all_by_type('Actor') == 5)
 b:destroy()
 assert(not b:is_alive() and b:get('Rotation') == nil)
 assert(not pcall(function() world.spawn('MissingType',0,0) end))
 assert(not pcall(function() world.spawn('prefabs/missing.prefab.json',0,0) end))
 assert(not pcall(function() world.find_in_radius(0,0,-1) end))
 self.actor:set_rotation(99)
end }
)");
		Healthy(script);
		Check(owner->GetActorRotation() == 99.f, "World/handle assertions did not finish");
		const auto saved = ly::SceneSerializer::Save(world);
		Check(saved["entities"].size() == 4, "Lua spawns must have scene identities; destroyed spawns omitted");
		Check(world.unresolvedPrefabs.empty(), "Rejected runtime prefab spawns must not become unresolved authored entities");
		ScriptRuntime::Get().SetPlayerResolver([other = Named(foreign, "WrongWorld")](ly::World&) { return other; });
		auto& wrong = Attach(*owner, "wrong-player", "function on_start(self) assert(world.player()==nil) end");
		Healthy(wrong);
		ScriptRuntime::Get().SetPlayerResolver({});
	}
	void Events()
	{
		ly::World world{ nullptr };
		auto actor = world.SpawnActor<TestBody>().lock();
		auto other = world.SpawnActor<TestBody>().lock();
		other->SetActorLocation({ 300, 0 });
		world.TickInternal(0.f);
		auto& script = Attach(*actor, "events", R"(
local starts, begins, ends, hits = 0,0,0,0
return {
 on_start=function(self) starts=starts+1; self.actor:set_rotation(starts) end,
 on_create=function(self) error('on_create must be a fallback') end,
 on_overlap_begin=function(self,other) assert(other:is_alive()); begins=begins+1; self.actor:set_location(begins,ends) end,
 on_overlap_end=function(self,other) ends=ends+1; self.actor:set_location(begins,ends) end,
 on_hit=function(self,other) hits=hits+1; self.actor:set_rotation(hits+10) end,
 on_destroy=function(self) self.actor:set_velocity(17,18) end
}
)");
		world.StepPhysics(1.f/60.f);
		other->SetActorLocation({ 5, 0 });
		for (int i = 0; i < 3; ++i) world.StepPhysics(1.f/60.f);
		Check(actor->GetActorLocation() == sf::Vector2f{ 1, 0 } && actor->GetActorRotation() == 11.f, "Physical begin/hit must fire once");
		other->SetActorLocation({ 300, 0 });
		for (int i = 0; i < 3; ++i) world.StepPhysics(1.f/60.f);
		Check(actor->GetActorLocation() == sf::Vector2f{ 1, 1 }, "Physical end must fire once");
		// Filters and body removal may prevent Box2D from delivering an end event.
		for (bool disable : { false, true })
		{
			const auto begins = actor->GetActorRotation() - 10.f;
			other->SetActorLocation({ 5, 0 });
			for (int i = 0; i < 3; ++i) world.StepPhysics(1.f/60.f);
			const auto touching = actor->GetActorLocation();
			Check(actor->GetActorRotation() == begins + 11.f, "Retouch begin/hit did not fire");
			if (disable) other->SetEnablePhysics(false);
			else other->SetCollisionMask(CollisionLayer::None);
			Check(actor->GetActorLocation().y == touching.y + 1, "Invalidating a contact must end the other component's overlap");
			other->SetActorLocation({ 300, 0 });
			for (int i = 0; i < 3; ++i) world.StepPhysics(1.f/60.f);
			if (disable) other->SetEnablePhysics(true);
			else other->SetCollisionMask(CollisionLayer::Layer0);
			other->SetActorLocation({ 5, 0 });
			for (int i = 0; i < 3; ++i) world.StepPhysics(1.f/60.f);
			Check(actor->GetActorRotation() == begins + 12.f, "Restored filter/body must fire begin/hit again");
			other->SetActorLocation({ 300, 0 });
			for (int i = 0; i < 3; ++i) world.StepPhysics(1.f/60.f);
		}
		// Reset this fixture's script counters before compound-contact checks.
		Check(script.Reload(), "Event fixture reload failed");
		auto* resetCollision = actor->FindComponent<ly::CollisionComponent>();
		resetCollision->NotifyContact(other.get(), true);
		resetCollision->NotifyContact(other.get(), false);
		// Simulate the multiple shape contacts emitted by compound bodies.
		auto* collision = actor->FindComponent<ly::CollisionComponent>();
		collision->NotifyContact(other.get(), true);
		collision->NotifyContact(other.get(), true);
		collision->NotifyContact(other.get(), false);
		Check(actor->GetActorLocation() == sf::Vector2f{ 2, 1 }, "Compound contact ended too early");
		collision->NotifyContact(other.get(), false);
		Check(actor->GetActorLocation() == sf::Vector2f{ 2, 2 }, "Compound contacts must deduplicate begin/end");
		collision->NotifyContact(other.get(), true);
		other->Destroy();
		Check(actor->GetActorLocation() == sf::Vector2f{ 3, 3 }, "Destroying a contact actor must end the overlap");
		Healthy(script);
		actor->Destroy();
		Check(actor->GetVelocity() == sf::Vector2f{ 17, 18 }, "Script table teardown missing");
	}
	void Audio()
	{
		std::vector<std::int16_t> silence(48000 * 10, 0);
		sf::SoundBuffer buffer;
		Check(buffer.loadFromSamples(silence.data(), silence.size(), 1, 48000, { sf::SoundChannel::Mono }) && buffer.saveToFile(root / "silence.wav"), "Cannot generate silent audio fixture");
		ly::World world{ nullptr };
		auto actor = Named(world, "Audio");
		world.TickInternal(0.f);
		auto& script = Attach(*actor, "audio", R"(
local handle
function on_start(self)
 assert(not pcall(audio.play,'../outside.wav'))
 assert(not pcall(audio.play,'C:/outside.wav'))
 assert(not pcall(audio.play,'/outside.wav'))
 assert(not pcall(audio.play,'missing.wav'))
 assert(not pcall(audio.play,'silence.wav',0/0))
 assert(not pcall(audio.play,'silence.wav',20,0))
 assert(not pcall(music.fade,'silence.wav',-1))
 local stopped=audio.play('silence.wav',0,1.2)
 assert(stopped:is_playing()); stopped:stop(); stopped:stop()
 assert(not stopped:is_playing())
 handle=audio.play('silence.wav',0)
 music.play('silence.wav',0)
 music.fade('silence.wav',0.1,0)
end
)");
		Healthy(script);
		auto& manager = actor->GetEngineContext().Audio();
		Check(manager.PlayingSoundCount() == 1, "Lua sound handle did not stop its voice");
		const auto revision = manager.MusicRevision();
		manager.Update(.2f); manager.Update(.2f);
		Check(manager.MusicRevision() == revision && manager.IsMusicPlaying(), "Fade continuation lost music ownership");
		Check(script.Reload(), "Audio script reload failed");
		Check(manager.PlayingSoundCount() == 1, "Reload leaked an old sound");
		auto next = Named(world, "Next music owner");
		world.TickInternal(0.f);
		auto& nextScript = Attach(*next, "next-music", "function on_start(self) music.fade('silence.wav',0.1,0) end");
		Healthy(nextScript);
		actor->Destroy();
		Check(manager.PlayingSoundCount() == 0 && manager.IsMusicPlaying(), "Destroy stopped another script's music or leaked its sound");
		next->Destroy();
		Check(!manager.IsMusicPlaying(), "Destroy left owned music playing");
		manager.Update(1.f);
		Check(!manager.IsMusicPlaying(), "Canceled fade restarted music");
		manager.CleanCycle();
		manager.FadeToMusic((root / "silence.wav").generic_u8string(), AudioType::Music, 0.f);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		const auto offset = manager.MusicPlayingOffset();
		Check(offset > sf::Time::Zero, "Music playback must advance before the repeat-fade check");
		manager.Update(.2f);
		manager.FadeToMusic((root / "silence.wav").generic_u8string(), AudioType::Music, 0.f);
		const auto fadeRevision = manager.MusicRevision();
		manager.Update(5.f);
		Check(manager.MusicPlayingOffset() >= offset && manager.IsMusicPlaying(), "Same-track fade-in must not restart playback");
		manager.FadeToMusic((root / "silence.wav").generic_u8string(), AudioType::Music, 0.f);
		Check(manager.MusicRevision() != fadeRevision && manager.MusicPlayingOffset() >= offset, "Unchanged music must transfer Lua ownership without restarting");
		manager.StopMusic();
		const auto first = manager.PlaySoundTracked((root / "silence.wav").generic_string(), AudioType::SFX_UI, 0.f);
		manager.StopSound(first); manager.CleanCycle();
		const auto second = manager.PlaySoundTracked((root / "silence.wav").generic_string(), AudioType::SFX_UI, 0.f);
		manager.StopSound(first);
		Check(manager.IsSoundPlaying(second), "Stale handle stopped a reused pooled voice");
		manager.StopSound(second);
	}
	void Timers()
	{
		ly::World world{ nullptr };
		auto actor = Named(world, "Timer");
		world.TickInternal(0.f);
		auto& script = Attach(*actor, "timers", R"(
function on_start(self)
 local canceled=timer.after(0.05,function() error('canceled timer fired') end)
 canceled:cancel(); assert(not canceled:is_active())
 local repeatHandle
 repeatHandle=timer.every(0.1,function()
   self.actor:set_rotation(self.actor:rotation()+1)
   if self.actor:rotation() == 2 then repeatHandle:cancel() end
 end)
 timer.after(0.2,function() self.actor:set_location(7,8) end)
 timer.after(10,function() self.actor:set_velocity(999,999) end)
 assert(not pcall(function() timer.every(0,function() end) end))
end
)");
		world.SetPaused(true);
		world.TickInternal(1.f);
		Check(actor->GetActorRotation() == 0.f, "Timers advanced while paused");
		world.SetPaused(false);
		world.TickInternal(0.1f);
		world.TickInternal(0.1f);
		world.TickInternal(1.f);
		Check(actor->GetActorRotation() == 2.f && actor->GetActorLocation() == sf::Vector2f{ 7,8 }, "after/every/cancel failed");
		Write("timers.lua", "function on_start(self) self.actor:set_rotation(42) end");
		Check(script.Reload(), "Timer script reload failed");
		world.TickInternal(20.f);
		Check(actor->GetVelocity() != sf::Vector2f{ 999,999 } && actor->GetActorRotation() == 42.f, "Timers survived reload");
		auto& destroy = Attach(*actor, "timer-destroy", R"(
function on_start(self)
 timer.after(0,function() self.actor:destroy() end)
 timer.after(0,function() error('timer survived destroy') end)
end
)");
		world.TickInternal(0.1f);
		Healthy(script); Healthy(destroy);
		Check(actor->GetIsPendingDestroy(), "Destroy from timer failed");
		auto limited = Named(world, "TimerLimit");
		ScriptRuntime::Get().SetInstructionBudget(10000);
		auto& loop = Attach(*limited, "timer-loop", "function on_start(self) timer.after(0,function() while true do end end) end");
		world.TickInternal(0.f);
		Check(loop.LastError().find("instruction budget") != std::string::npos, "Timer callbacks bypass instruction limits");
		ScriptRuntime::Get().SetInstructionBudget(10000000);
	}
	void Messages()
	{
		ly::World world{ nullptr };
		auto receiver = Named(world, "Receiver"), sender = Named(world, "Sender");
		world.TickInternal(0.f);
		auto& target = Attach(*receiver, "receiver", R"(
local count=0
function on_message(self,name,data)
 assert(name=='activate' and data.nested.value==7 and data.text=='hello')
 count=count+1
 self.actor:set_rotation(count)
 data.nested.value=99
end
)");
		auto& source = Attach(*sender, "sender", R"(
function on_start(self)
 local data={nested={value=7},text='hello'}
 assert(world.find('Receiver'):send('activate',data))
 assert(world.find('Receiver'):send('activate',data))
 assert(data.nested.value==7)
 assert(not pcall(function() self.actor:send('bad',{fn=function() end}) end))
 local cycle={}; cycle.self=cycle
 assert(not pcall(function() self.actor:send('cycle',cycle) end))
end
)");
		Check(receiver->GetActorRotation() == 0.f, "Messages must defer instead of recursively entering Lua");
		world.TickInternal(0.f);
		Check(receiver->GetActorRotation() == 2.f, "Message payloads must copy between deliveries");
		Check(target.QueueMessage("activate", { { "nested", { { "value", 7 } } }, { "text", "hello" } }), "Could not queue C++ message");
		Check(target.Reload(), "Message reload failed");
		world.TickInternal(0.f);
		Check(receiver->GetActorRotation() == 2.f, "Reload must discard queued messages");
		Healthy(target); Healthy(source);
		ScriptRuntime::Get().SetInstructionBudget(10000);
		auto& loop = Attach(*sender, "message-loop", "function on_message(self,name,data) while true do end end");
		loop.QueueMessage("loop", nullptr);
		world.TickInternal(0.f);
		Check(loop.LastError().find("instruction budget") != std::string::npos, "Message callbacks bypass instruction limits");
		ScriptRuntime::Get().SetInstructionBudget(10000000);
	}
	void InputVectors()
	{
		InputViewport viewport;
		ly::World world{ nullptr };
		world.SetRenderViewport(&viewport);
		world.SetViewOverride(sf::View{ { 100,200 }, { 800,600 } });
		bool held = false;
		ScriptRuntime::Get().SetKeyReader([&](const std::string& name) { return name == "A" && held; });
		auto actor = Named(world, "Input");
		world.TickInternal(0.f);
		auto& script = Attach(*actor, "input-vectors", R"(
local presses,releases=0,0
function on_start(self)
 assert(not input.pressed('A') and not input.released('A'))
 local a=vec(3,4)
 assert(a:length()==5 and a:dot(vec(2,1))==10)
 assert((a+vec(1,2)).y==6 and (a-vec(1,2)).x==2 and (a*2).x==6 and (2*a).y==8)
 assert(math.abs(a:normalize():length()-1)<0.0001 and vec(0,0):normalize():length()==0)
 assert(self.actor:set('Location',vec(3,4)))
 local mouse=input.mouse_world(); assert(mouse.x==107 and mouse.y==211)
end
function on_update(self,dt)
 if input.pressed('A') then presses=presses+1; assert(input.pressed('A')) end
 if input.released('A') then releases=releases+1 end
 self.actor:set_rotation(presses*10+releases)
end
)");
		held = true;
		world.TickInternal(0.f);
		world.TickInternal(0.f);
		Check(actor->GetActorRotation() == 10.f, "Pressed must be stable within a tick and fire once");
		held = false;
		world.TickInternal(0.f);
		world.TickInternal(0.f);
		Check(actor->GetActorRotation() == 11.f, "Released must fire once");
		viewport.focused = false;
		held = true;
		world.TickInternal(0.f);
		Check(actor->GetActorRotation() == 11.f, "Input must respect viewport focus");
		Healthy(script);
		ScriptRuntime::Get().SetKeyReader({});
	}
	void LuaTypes()
	{
		std::string error;
		const auto output = root / ".vega/vega.lua";
		Check(vega::script::GenerateLuaTypes(output, error), error);
		std::ifstream input{ output };
		const std::string text{ std::istreambuf_iterator<char>{ input }, {} };
		for (const auto* type : ly::reflect::TypeRegistry::Get().RegisteredTypes())
		{
			Check(text.find("-- Reflected type: " + type->name) != std::string::npos, "Missing annotation type " + type->name);
			for (const auto* property : type->AllProperties())
				Check(text.find(nlohmann::json(property->name).dump()) != std::string::npos, "Missing property literal " + property->name);
		}
		for (const char* api : { "world.spawn", "world.find_in_radius", "timer.every", "input.pressed", "on_overlap_begin", "ActorHandle:send", "VegaVector:normalize" })
			Check(text.find(api) != std::string::npos, std::string{ "Missing API annotation " } + api);
	}
	void ExamplesCost()
	{
		// Same three script hosts, plus a player and three spawned wave actors in
		// each world. Warm up first; alternate order to reduce drift in Debug runs.
		ly::World baseline{ nullptr }, scripted{ nullptr };
		auto setup = [](ly::World& world)
		{
			Named(world, "Trigger")->AddComponent<ly::CollisionComponent>();
			Named(world, "Hazard"); Named(world, "Receiver");
			Named(world, "Player");
			world.TickInternal(0.f);
		};
		setup(baseline); setup(scripted);
		for (int index = 0; index < 3; ++index) Named(baseline, "Wave");
		auto actors = scripted.GetActorsByTypeIncludingPending<ly::Actor>();
		auto& trigger = Attach(*actors[0].lock(), "example-trigger", "local done=false\nfunction on_overlap_begin(self,other) if not done and other:name()=='Player' then done=true; for i=1,3 do world.spawn('LuaActor',i*100,0) end; world.find('Receiver'):send('wave',{count=3}) end end");
		auto& hazard = Attach(*actors[1].lock(), "example-hazard", "function on_start(self) timer.every(0.02,function() self.actor:set_rotation(self.actor:rotation()+2) end) end");
		auto& receiver = Attach(*actors[2].lock(), "example-receiver", "function on_message(self,name,data) if name=='wave' then self.actor:set_rotation(data.count) end end");
		actors[0].lock()->FindComponent<ly::CollisionComponent>()->NotifyContact(actors[3].lock().get(), true);
		scripted.TickInternal(0.f);
		Check(scripted.GetActorCountIncludingPending() == baseline.GetActorCountIncludingPending() &&
			actors[2].lock()->GetActorRotation() == 3.f, "Three-example setup did not spawn/message the wave");
		for (int i = 0; i < 1000; ++i) { baseline.TickInternal(1.f/60.f); scripted.TickInternal(1.f/60.f); }
		using Clock = std::chrono::steady_clock;
		std::vector<double> without, with;
		for (int batch = 0; batch < 9; ++batch)
		{
			const auto measure = [](ly::World& world)
			{
				const auto start = Clock::now();
				for (int i = 0; i < 3000; ++i) world.TickInternal(1.f/60.f);
				return std::chrono::duration<double, std::micro>(Clock::now() - start).count() / 3000.;
			};
			if (batch % 2) { with.push_back(measure(scripted)); without.push_back(measure(baseline)); }
			else { without.push_back(measure(baseline)); with.push_back(measure(scripted)); }
		}
		std::sort(without.begin(), without.end()); std::sort(with.begin(), with.end());
		std::cout << "Three-example World::TickInternal median microseconds/frame: without=" << without[4]
			<< " with=" << with[4] << " delta=" << with[4] - without[4] << " (Debug, 9 x 3000 ticks, hot reload off, no rendering/limiter)\n";
		Healthy(trigger); Healthy(hazard); Healthy(receiver);
	}
}

int main(int argc, char** argv)
{
	ly::EngineContext engineContext;
	ly::EngineContext::Scope contextScope{ engineContext };
	const std::string group = argc > 1 ? argv[1] : "World";
	root = std::filesystem::path{ VEGA_SCRIPT_TEST_ROOT } / group;
	std::filesystem::create_directories(root);
	vega::script::SetScriptRoot(root);
	ly::PrefabLibrary::Get().SetRoot(root);
	vega::script::RegisterScriptTypes();
	ly::reflect::Class<LuaActor, ly::Actor>("LuaActor").Spawnable();
	ScriptRuntime::Get().SetHotReload(false);
	if (group == "World") WorldApi();
	else if (group == "Hierarchy") Hierarchy();
	else if (group == "Events") Events();
	else if (group == "Timers") Timers();
	else if (group == "Audio") Audio();
	else if (group == "Messages") Messages();
	else if (group == "InputVectors") InputVectors();
	else if (group == "LuaTypes") LuaTypes();
	else if (group == "ExamplesCost") ExamplesCost();
	else Check(false, "Unknown group " + group);
	std::cout << "VegaScript " << group << ": " << failures << " failures\n";
	return failures ? 1 : 0;
}
