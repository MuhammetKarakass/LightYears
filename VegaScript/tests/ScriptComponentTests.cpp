#include "framework/EngineContext.h"
#include <script/ScriptComponent.h>
#include <script/ScriptRuntime.h>
#include <framework/Actor.h>
#include <framework/World.h>
#include <framework/CollisionComponent.h>
#include <framework/scene/Scene.h>
#include <framework/scene/SceneEntityComponent.h>
#include <framework/debug/LogBuffer.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

namespace
{
	int failures = 0;
	class ReflectedActor : public vega::Actor
	{
	public:
		explicit ReflectedActor(vega::World* world) : Actor{ world } {}
		float GetPower() const { return power; }
		void SetPower(float value) { power = value; }
	private:
		float power = 1.f;
	};
	class ThrowingActor : public ReflectedActor
	{
	public:
		using ReflectedActor::ReflectedActor;
	};
	const std::filesystem::path root{ VEGA_SCRIPT_TEST_ROOT };
	void Check(bool condition, const char* message)
	{
		if (!condition) { std::cerr << message << '\n'; ++failures; }
	}
	void Write(const std::string& path, const std::string& source)
	{
		std::ofstream output{ root / path, std::ios::trunc };
		output << source;
		output.close();
		// Filesystems differ in timestamp granularity; make every edit observable.
		static int revision = 0;
		std::filesystem::last_write_time(root / path, std::filesystem::file_time_type::clock::now() + std::chrono::seconds(++revision));
	}
	float Number(const vega::script::ScriptComponent& component, const std::string& name)
	{
		const auto entry = component.Properties().find(name);
		return entry != component.Properties().end() && std::holds_alternative<float>(entry->second) ? std::get<float>(entry->second) : -999.f;
	}
	void Poll(vega::World& world)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(510));
		world.TickInternal(0.f);
	}
}

int main()
{
	vega::EngineContext engineContext;
	vega::EngineContext::Scope contextScope{ engineContext };
	using namespace vega::script;
	std::filesystem::create_directories(root);
	SetScriptRoot(root);
	RegisterScriptTypes();
	{
		vega::debug::LogCapture capture;
		vega::World diagnosticsWorld{ nullptr };
		auto diagnosticActor = diagnosticsWorld.SpawnActor<vega::Actor>().lock();
		diagnosticsWorld.TickInternal(0.f);
		Write("console-print.lua", "function on_create(self)\n print('unit hello', 42, true)\nend\n");
		diagnosticActor->AddComponent<ScriptComponent>("console-print.lua");
		bool printed = false;
		for (const auto& row : capture.Buffer().Snapshot())
			if (row.channel == vega::debug::LogChannel::Lua && row.message == "unit hello\t42\ttrue" && row.line == 2 && row.file.find("console-print.lua") != std::string::npos) printed = true;
		Check(printed, "Lua print must retain arguments and source line");
		Write("console-runtime.lua", "function on_create(self)\n error('unit runtime failure')\nend\n");
		diagnosticActor->AddComponent<ScriptComponent>("console-runtime.lua");
		Write("console-load.lua", "local ok = true\nlocal invalid = )\n");
		diagnosticActor->AddComponent<ScriptComponent>("console-load.lua");
		bool runtime = false, load = false;
		for (const auto& row : capture.Buffer().Snapshot())
		{
			if (row.level != vega::debug::LogLevel::Error || row.channel != vega::debug::LogChannel::Lua || row.line != 2) continue;
			runtime |= row.file == "console-runtime.lua" && row.message.find("unit runtime failure") != std::string::npos;
			load |= row.file == "console-load.lua";
		}
		Check(runtime && load, "Lua runtime/load errors must retain script source line");
	}
	ScriptRuntime::Get().SetHotReload(true);
	vega::World world{ nullptr };
	auto actor = world.SpawnActor<vega::Actor>().lock();
	world.TickInternal(0.f);
	Write("lifecycle.lua", R"(
properties = { speed = 12, enabled = true, color = "#ff8800", offset = {x=2,y=3} }
local order = ""
function on_create(self)
    order = order .. "C"
    assert(self.props.speed == 12 or self.props.speed == 90)
    self.actor:set_rotation(10)
end
function on_update(self, dt)
    assert(order == "C" or order == "CU")
    order = "CU"
    self.actor:set_rotation(self.actor:rotation() + self.props.speed * dt)
end
function on_destroy(self)
    assert(order == "CU" or order == "C")
    self.actor:set_location(123, 456)
end
)");
	auto& component = actor->AddComponent<ScriptComponent>("lifecycle.lua");
	Check(component.LastError().empty() && actor->GetActorRotation() == 10.f, "on_create did not run immediately");
	Check(Number(component, "speed") == 12.f && component.Properties().size() == 4, "Property defaults/types missing");
	Check(std::get<bool>(component.Properties().at("enabled")) && std::get<std::string>(component.Properties().at("color")) == "#ff8800" &&
		std::get<sf::Vector2f>(component.Properties().at("offset")) == sf::Vector2f{ 2.f, 3.f }, "Non-number defaults incorrect");
	Check(component.SetProperty("speed", 90.f), "Override refused");
	Check(component.SetProperty("enabled", false) && component.SetProperty("color", std::string{"blue"}) &&
		component.SetProperty("offset", sf::Vector2f{8.f,9.f}), "Non-number overrides refused");
	world.TickInternal(1.f);
	Check(actor->GetActorRotation() == 100.f, "on_update did not use self.props override/dt");
	world.SetPaused(true);
	world.TickInternal(1.f);
	Check(actor->GetActorRotation() == 100.f, "Script ticked while paused");
	world.SetPaused(false);
	Check(component.Reload() && Number(component, "speed") == 90.f && actor->GetActorLocation() == sf::Vector2f{123.f,456.f}, "Reload lost override or skipped on_destroy");
	Check(!std::get<bool>(component.Properties().at("enabled")) && std::get<std::string>(component.Properties().at("color")) == "blue" &&
		std::get<sf::Vector2f>(component.Properties().at("offset")) == sf::Vector2f{8.f,9.f}, "Reload lost non-number overrides");
	actor->Destroy();
	Check(component.LastError().empty() && actor->GetActorLocation() == sf::Vector2f{123.f,456.f}, "Lifecycle callback order failed");

	auto hotActor = world.SpawnActor<vega::Actor>().lock();
	world.TickInternal(0.f);
	Write("hot.lua", "properties={speed=1}\nfunction on_create(self) self.actor:set_rotation(self.props.speed) end");
	auto& hot = hotActor->AddComponent<ScriptComponent>("hot.lua");
	hot.SetProperty("speed", 7.f);
	Write("hot.lua", "properties={speed=2,added=true}\nfunction on_create(self) self.actor:set_rotation(self.props.speed+10) end");
	world.TickInternal(0.f);
	Check(hotActor->GetActorRotation() == 1.f, "Hot reload polled before 0.5 seconds");
	Poll(world);
	Check(hot.LastError().empty() && hotActor->GetActorRotation() == 17.f && Number(hot,"speed") == 7.f, "Hot reload did not preserve override");
	Write("hot.lua", "function broken(\n");
	Poll(world);
	Check(hot.LastError().find("hot.lua:") != std::string::npos && hot.LastError().find(":2:") != std::string::npos, "Syntax error missing file:line");
	Write("hot.lua", "function on_update(self,dt)\n error('runtime failure')\nend");
	Poll(world);
	Check(hot.LastError().find("hot.lua:2:") != std::string::npos && hot.LastError().find("runtime failure") != std::string::npos, "Runtime error missing file:line");
	const auto error = hot.LastError();
	world.TickInternal(1.f);
	Check(hot.LastError() == error, "Error did not suspend script");
	Write("hot.lua", "function on_update(self,dt) self.actor:set_rotation(42) end");
	Poll(world);
	Check(hot.LastError().empty() && hotActor->GetActorRotation() == 42.f, "Fixed file did not recover");

	Write("api.lua", R"(
assert(io == nil and os == nil and package == nil and debug == nil)
assert(loadfile == nil and dofile == nil and load == nil and getmetatable == nil)
assert(_G.io == nil)
assert(not input.key_down("A"))
assert(private_global == nil and math.private_global == nil)
private_global = 99
math.private_global = 99
function on_create(self)
    assert(self.actor:is_valid() and self.actor:id() > 0 and self.actor:type() == "Actor")
    assert(self.actor:set("Rotation", 33))
    assert(self.actor:get("Rotation") == 33)
    assert(self.actor:set("Location", {x=4,y=5}))
    assert(not self.actor:set("Velocity", {x=0,y=0})) -- reflected read-only
    assert(self.actor:component("Missing") == nil)
    local c = self.actor:component("ScriptComponent")
    assert(type(c:get("Script")) == "string")
    assert(not c:set("Script", "other.lua"))
    assert(c:set("Active", true))
    self.actor:set_velocity(6,7)
    assert(self.actor:velocity().y == 7 and time() >= 0)
end
)");
	auto& api = hotActor->AddComponent<ScriptComponent>("api.lua");
	Check(api.LastError().empty() && hotActor->GetActorRotation() == 33.f && hotActor->GetActorLocation() == sf::Vector2f{4.f,5.f}, "Sandbox/reflection/handle API failed");
	auto otherActor = world.SpawnActor<vega::Actor>().lock();
	world.TickInternal(0.f);
	auto& isolated = otherActor->AddComponent<ScriptComponent>("api.lua");
	Check(isolated.LastError().empty(), "Environments leaked globals/library mutations");

	Write("command-prefix.lua", "properties={}");
	Write("component-handle.lua", R"(
local target
function on_create(self)
    target = self.actor:component("CollisionComponent")
    assert(target:get("Collision Mask") == 5)
end
function on_update(self, dt)
    assert(target:set("Collision Mask", 4))
    assert(not target:set("Collision Mask", 256))
    assert(target:get("Collision Mask") == 4)
end
)");
	auto stableActor = world.SpawnActor<vega::Actor>().lock();
	// A script precedes the target; more components arrive after the handle is retained.
	stableActor->AddComponent<ScriptComponent>("command-prefix.lua");
	auto& stableCollision = stableActor->AddComponent<vega::CollisionComponent>();
	stableCollision.SetCollisionMask(static_cast<CollisionLayer>(5));
	world.TickInternal(0.f);
	auto& stableScript = stableActor->AddComponent<ScriptComponent>("component-handle.lua");
	stableActor->AddComponent<vega::ActorComponent>();
	stableActor->AddComponent<ScriptComponent>("command-prefix.lua");
	world.TickInternal(0.1f);
	Check(stableScript.LastError().empty() && static_cast<int>(stableCollision.GetCollisionMask()) == 4,
		"Retained Lua component handle must target its type after other components are added");

	Write("destroy.lua", R"(
function on_update(self,dt)
    self.actor:destroy()
    assert(not self.actor:is_valid())
    assert(self.actor:get("Rotation") == nil)
    assert(not self.actor:set("Rotation", 10))
    self.actor:set_location(999,999)
end
function on_destroy(self) assert(not self.actor:is_valid()) end
)");
	auto& destroyed = otherActor->AddComponent<ScriptComponent>("destroy.lua");
	world.TickInternal(0.1f);
	Check(otherActor->GetIsPendingDestroy() && destroyed.LastError().empty() && otherActor->GetActorLocation() != sf::Vector2f{999.f,999.f}, "Handle remained valid after destroy/reentrant lifecycle failed");

	auto& escape = hotActor->AddComponent<ScriptComponent>("../outside.lua");
	Check(!escape.LastError().empty(), "Asset root traversal accepted");

	vega::reflect::Class<ReflectedActor, vega::Actor>("ReflectedActor")
		.Spawnable()
		.Property("Power", &ReflectedActor::GetPower, &ReflectedActor::SetPower);
	auto derived = world.SpawnActor<ReflectedActor>().lock();
	world.TickInternal(0.f);
	Write("derived.lua", "function on_create(self) assert(self.actor:type()=='ReflectedActor'); assert(self.actor:set('Power',77)); assert(self.actor:get('Power')==77) end");
	auto& reflected = derived->AddComponent<ScriptComponent>("derived.lua");
	Check(reflected.LastError().empty() && derived->GetPower() == 77.f, "Lua did not use most-derived reflection");
	vega::reflect::Class<ThrowingActor, ReflectedActor>("ThrowingActor")
		.Property("Throwing", &ReflectedActor::GetPower, [](ThrowingActor&, float) { throw std::runtime_error("Lua setter refused"); });
	auto throwingActor = world.SpawnActor<ThrowingActor>().lock();
	throwingActor->SetPower(77.f);
	world.TickInternal(0.f);
	Write("setter-error.lua", "function on_create(self) local ok, reason = self.actor:set('Throwing', 99); assert(not ok and reason == 'Lua setter refused'); assert(self.actor:get('Power') == 77) end");
	auto& setterError = throwingActor->AddComponent<ScriptComponent>("setter-error.lua");
	Check(setterError.LastError().empty(), "Lua must receive a setter failure and its reason without unwinding the callback");
	Write("create-error.lua", "function on_create(self)\n error('create failure')\nend\nfunction on_update(self,dt) self.actor:set_rotation(999) end");
	auto& createError = derived->AddComponent<ScriptComponent>("create-error.lua");
	world.TickInternal(0.f);
	Check(createError.LastError().find("create-error.lua:2:") != std::string::npos && derived->GetActorRotation() != 999.f, "Create error did not suspend updates");
	Write("destroy-error.lua", "function on_destroy(self)\n error('destroy failure')\nend");
	auto& destroyError = derived->AddComponent<ScriptComponent>("destroy-error.lua");
	Check(!destroyError.Reload() && destroyError.LastError().find("destroy-error.lua:2:") != std::string::npos, "Teardown error was not contained/stored");
	Write("destroy-error.lua", "function on_create(self) self.actor:set_rotation(88) end");
	Poll(world);
	Check(destroyError.LastError().empty() && derived->GetActorRotation() == 88.f, "Teardown error did not recover on file change");
	derived->Destroy();
	auto teardownActor = world.SpawnActor<vega::Actor>().lock();
	world.TickInternal(0.f);
	Write("reentrant.lua", "local calls=0\nfunction on_destroy(self) calls=calls+1; assert(calls==1); self.actor:destroy() end");
	auto& reentrant = teardownActor->AddComponent<ScriptComponent>("reentrant.lua");
	teardownActor->Destroy();
	Check(teardownActor->GetIsPendingDestroy() && reentrant.LastError().empty(), "Destroy from on_destroy recursively entered actor teardown");
	auto reloadActor = world.SpawnActor<vega::Actor>().lock();
	world.TickInternal(0.f);
	auto& reloadDestroy = reloadActor->AddComponent<ScriptComponent>("reentrant.lua");
	reloadDestroy.Reload();
	Check(reloadActor->GetIsPendingDestroy() && reloadDestroy.LastError().empty(), "Reload teardown must not call on_destroy twice");
	hotActor->Destroy();
	Write("staged.lua", R"(
properties = { speed = 1, enabled = true, text = "default", offset = {x=0,y=0} }
function on_create(self)
	assert(self.props.speed == 27 and not self.props.enabled)
	assert(self.props.text == "authored" and self.props.offset.x == 3 and self.props.offset.y == 4)
	self.actor:set_rotation(self.props.speed)
end
)");
	vega::World stagedWorld{ nullptr };
	auto stagedActor = stagedWorld.SpawnActor<ReflectedActor>().lock();
	stagedActor->AddComponent<vega::SceneEntityComponent>("", "Script round trip");
	auto& staged = stagedActor->AddComponent<ScriptComponent>("staged.lua");
	Check(staged.SetOverrides({ { "speed", 27.f }, { "enabled", false }, { "text", std::string{ "authored" } },
		{ "offset", sf::Vector2f{ 3.f, 4.f } } }), "Could not stage pre-BeginPlay overrides");
	Check(!staged.SetOverrides({ { "invalid", 5 } }), "Unsupported override type was accepted");
	const auto scene = vega::SceneSerializer::Save(stagedWorld);
	vega::World restoredWorld{ nullptr };
	const auto restoredResult = vega::SceneSerializer::Load(restoredWorld, scene);
	Check(restoredResult.spawned == 1 && restoredResult.warnings.empty(), "Script scene failed to load");
	Check(vega::SceneSerializer::Save(restoredWorld) == scene, "Script scene round trip changed overrides");
	const auto restoredActor = restoredWorld.GetActorsByTypeIncludingPending<ReflectedActor>()[0].lock();
	Check(restoredActor->GetActorRotation() == 0.f, "Preparing declarations invoked on_create before promotion");
	restoredWorld.TickInternal(0.f);
	const auto* restoredScript = restoredActor->FindComponent<ScriptComponent>();
	Check(restoredScript->LastError().empty() && restoredActor->GetActorRotation() == 27.f, "on_create did not see the staged overrides");
	auto invalidScene = scene;
	invalidScene["entities"][0]["scripts"][0]["properties"] = { { "missing", 3 }, { "speed", true } };
	vega::World invalidWorld{ nullptr };
	Check(vega::SceneSerializer::Load(invalidWorld, invalidScene).warnings.size() == 2, "Unknown/type-mismatched script overrides must warn");
	invalidScene["entities"][0]["scripts"][0]["path"] = "../outside.lua";
	vega::World invalidPathWorld{ nullptr };
	Check(!vega::SceneSerializer::Load(invalidPathWorld, invalidScene).warnings.empty(), "Invalid script paths must warn during scene loading");
	// Keep runaway regressions small; every callback receives a fresh allowance.
	ScriptRuntime::Get().SetHotReload(false);
	ScriptRuntime::Get().SetInstructionBudget(10000);
	vega::World limitedWorld{ nullptr };
	auto limitedActor = limitedWorld.SpawnActor<vega::Actor>().lock();
	limitedWorld.TickInternal(0.f);
	Write("loop-update.lua", "function on_update(self,dt) while true do end end");
	auto& loopUpdate = limitedActor->AddComponent<ScriptComponent>("loop-update.lua");
	limitedWorld.TickInternal(0.f);
	Check(loopUpdate.LastError().find("script exceeded its instruction budget") != std::string::npos, "Infinite update must hit its instruction budget");
	const auto loopError = loopUpdate.LastError();
	limitedWorld.TickInternal(0.f);
	Check(loopUpdate.LastError() == loopError, "Budget failure must suspend the callback");
	Write("loop-chunk.lua", "while true do end");
	auto& loopChunk = limitedActor->AddComponent<ScriptComponent>("loop-chunk.lua");
	Check(loopChunk.LastError().find("script exceeded its instruction budget") != std::string::npos, "Infinite chunk must hit its instruction budget");
	Write("loop-create.lua", "function on_create(self) while true do end end");
	auto& loopCreate = limitedActor->AddComponent<ScriptComponent>("loop-create.lua");
	Check(loopCreate.LastError().find("script exceeded its instruction budget") != std::string::npos, "Infinite create must hit its instruction budget");
	Write("loop-destroy.lua", "function on_destroy(self) while true do end end");
	auto& loopDestroy = limitedActor->AddComponent<ScriptComponent>("loop-destroy.lua");
	Check(!loopDestroy.Reload() && loopDestroy.LastError().find("script exceeded its instruction budget") != std::string::npos, "Infinite teardown must hit its instruction budget");
	Write("budget-reset.lua", "local function work() local n=0; for i=1,3000 do n=n+i end end\nwork()\nfunction on_create(self) work() end\nfunction on_update(self,dt) work(); self.actor:set_rotation(self.actor:rotation()+1) end\nfunction on_destroy(self) work() end");
	auto& budgetReset = limitedActor->AddComponent<ScriptComponent>("budget-reset.lua");
	for (int i = 0; i < 4; ++i) limitedWorld.TickInternal(0.f);
	Check(budgetReset.LastError().empty() && limitedActor->GetActorRotation() == 4.f, "Instruction allowance must reset between chunk, create and repeated updates");
	ScriptRuntime::Get().SetInstructionBudget(10000000);
	ScriptRuntime::Get().SetMemoryLimit(4u * 1024u * 1024u);
	Write("memory-string.lua", "function on_update(self,dt) local s=string.rep('x', 16*1024*1024) end");
	auto& memoryString = limitedActor->AddComponent<ScriptComponent>("memory-string.lua");
	limitedWorld.TickInternal(0.f);
	Check(memoryString.LastError().find("not enough memory") != std::string::npos, "Runaway string allocation must hit the memory cap");
	Write("memory-table.lua", "local values={}\nwhile true do values[#values+1]=string.rep('x',1024) end");
	auto& memoryTable = limitedActor->AddComponent<ScriptComponent>("memory-table.lua");
	Check(memoryTable.LastError().find("not enough memory") != std::string::npos, "Runaway table growth in a chunk must hit the memory cap");
	const auto memoryError = memoryString.LastError();
	limitedWorld.TickInternal(0.f);
	Check(memoryString.LastError() == memoryError && budgetReset.LastError().empty(), "Memory failure must suspend the offender while other callbacks keep running");
	ScriptRuntime::Get().SetMemoryLimit(256u * 1024u * 1024u);
	Write("memory-string.lua", "function on_update(self,dt) self.actor:set_location(12,34) end");
	Check(memoryString.Reload(), "Script must be recoverable after a memory error");
	limitedWorld.TickInternal(0.f);
	Check(limitedActor->GetActorLocation() == sf::Vector2f{12.f,34.f}, "Runtime must keep executing after resource errors");
	std::cout << "VegaScript component tests: " << failures << " failures\n";
	return failures ? 1 : 0;
}
