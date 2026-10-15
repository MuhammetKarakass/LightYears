#include <script/ScriptComponent.h>
#include <script/ScriptRuntime.h>
#include <framework/Actor.h>
#include <framework/World.h>
#include <framework/scene/Scene.h>
#include <framework/scene/SceneEntityComponent.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

namespace
{
	int failures = 0;
	class ReflectedActor : public ly::Actor
	{
	public:
		explicit ReflectedActor(ly::World* world) : Actor{ world } {}
		float GetPower() const { return power; }
		void SetPower(float value) { power = value; }
	private:
		float power = 1.f;
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
	void Poll(ly::World& world)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(510));
		world.TickInternal(0.f);
	}
}

int main()
{
	using namespace vega::script;
	std::filesystem::create_directories(root);
	SetScriptRoot(root);
	RegisterScriptTypes();
	ScriptRuntime::Get().SetHotReload(true);
	ly::World world{ nullptr };
	auto actor = world.SpawnActor<ly::Actor>().lock();
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

	auto hotActor = world.SpawnActor<ly::Actor>().lock();
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
	auto otherActor = world.SpawnActor<ly::Actor>().lock();
	world.TickInternal(0.f);
	auto& isolated = otherActor->AddComponent<ScriptComponent>("api.lua");
	Check(isolated.LastError().empty(), "Environments leaked globals/library mutations");

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

	ly::reflect::Class<ReflectedActor, ly::Actor>("ReflectedActor")
		.Spawnable()
		.Property("Power", &ReflectedActor::GetPower, &ReflectedActor::SetPower);
	auto derived = world.SpawnActor<ReflectedActor>().lock();
	world.TickInternal(0.f);
	Write("derived.lua", "function on_create(self) assert(self.actor:type()=='ReflectedActor'); assert(self.actor:set('Power',77)); assert(self.actor:get('Power')==77) end");
	auto& reflected = derived->AddComponent<ScriptComponent>("derived.lua");
	Check(reflected.LastError().empty() && derived->GetPower() == 77.f, "Lua did not use most-derived reflection");
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
	auto teardownActor = world.SpawnActor<ly::Actor>().lock();
	world.TickInternal(0.f);
	Write("reentrant.lua", "local calls=0\nfunction on_destroy(self) calls=calls+1; assert(calls==1); self.actor:destroy() end");
	auto& reentrant = teardownActor->AddComponent<ScriptComponent>("reentrant.lua");
	teardownActor->Destroy();
	Check(teardownActor->GetIsPendingDestroy() && reentrant.LastError().empty(), "Destroy from on_destroy recursively entered actor teardown");
	auto reloadActor = world.SpawnActor<ly::Actor>().lock();
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
	ly::World stagedWorld{ nullptr };
	auto stagedActor = stagedWorld.SpawnActor<ReflectedActor>().lock();
	stagedActor->AddComponent<ly::SceneEntityComponent>("", "Script round trip");
	auto& staged = stagedActor->AddComponent<ScriptComponent>("staged.lua");
	Check(staged.SetOverrides({ { "speed", 27.f }, { "enabled", false }, { "text", std::string{ "authored" } },
		{ "offset", sf::Vector2f{ 3.f, 4.f } } }), "Could not stage pre-BeginPlay overrides");
	Check(!staged.SetOverrides({ { "invalid", 5 } }), "Unsupported override type was accepted");
	const auto scene = ly::SceneSerializer::Save(stagedWorld);
	ly::World restoredWorld{ nullptr };
	const auto restoredResult = ly::SceneSerializer::Load(restoredWorld, scene);
	Check(restoredResult.spawned == 1 && restoredResult.warnings.empty(), "Script scene failed to load");
	Check(ly::SceneSerializer::Save(restoredWorld) == scene, "Script scene round trip changed overrides");
	const auto restoredActor = restoredWorld.GetActorsByTypeIncludingPending<ReflectedActor>()[0].lock();
	Check(restoredActor->GetActorRotation() == 0.f, "Preparing declarations invoked on_create before promotion");
	restoredWorld.TickInternal(0.f);
	const auto* restoredScript = restoredActor->FindComponent<ScriptComponent>();
	Check(restoredScript->LastError().empty() && restoredActor->GetActorRotation() == 27.f, "on_create did not see the staged overrides");
	auto invalidScene = scene;
	invalidScene["entities"][0]["scripts"][0]["properties"] = { { "missing", 3 }, { "speed", true } };
	ly::World invalidWorld{ nullptr };
	Check(ly::SceneSerializer::Load(invalidWorld, invalidScene).warnings.size() == 2, "Unknown/type-mismatched script overrides must warn");
	invalidScene["entities"][0]["scripts"][0]["path"] = "../outside.lua";
	ly::World invalidPathWorld{ nullptr };
	Check(!ly::SceneSerializer::Load(invalidPathWorld, invalidScene).warnings.empty(), "Invalid script paths must warn during scene loading");
	std::cout << "VegaScript component tests: " << failures << " failures\n";
	return failures ? 1 : 0;
}
