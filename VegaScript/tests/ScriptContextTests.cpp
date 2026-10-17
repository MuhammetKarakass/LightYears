#include "framework/EngineContext.h"
#include "framework/World.h"
#include "framework/Actor.h"
#include "framework/ProjectSettings.h"
#include "framework/CollisionComponent.h"
#include "script/ScriptRuntime.h"
#include "script/ScriptComponent.h"
#include <filesystem>
#include <fstream>
#include <iostream>

int main()
{
	ly::EngineContext first, second;
	ly::ProjectSettings project;
	project.collisionLayers = { { 0, "FirstSolid", {} } };
	project.ApplyCollisionLayers(first);
	project.collisionLayers = { { 7, "SecondSensor", {} } };
	project.ApplyCollisionLayers(second);
	const std::filesystem::path root{ VEGA_SCRIPT_TEST_ROOT };
	for (const auto* name : { "first", "second" }) std::filesystem::create_directories(root / name);
	std::ofstream{ root / "first/shared.lua" } << R"(
function on_update(self, dt)
	local collision = self.actor:component('CollisionComponent')
	assert(collision:set('Collision Mask', 'FirstSolid'))
	assert(not collision:set('Collision Mask', 'SecondSensor'))
	self.actor:set_rotation(self.actor:rotation()+1)
end
)";
	std::ofstream{ root / "second/shared.lua" } << R"(
function on_update(self, dt)
	local collision = self.actor:component('CollisionComponent')
	assert(collision:set('Collision Mask', 'SecondSensor'))
	assert(not collision:set('Collision Mask', 'FirstSolid'))
	self.actor:set_rotation(self.actor:rotation()+2)
end
)";
	vega::script::RegisterScriptTypes(first);
	vega::script::RegisterScriptTypes(second);
	auto& firstRuntime = vega::script::ScriptRuntime::Get(first);
	auto& secondRuntime = vega::script::ScriptRuntime::Get(second);
	firstRuntime.SetRoot(root / "first"); secondRuntime.SetRoot(root / "second");
	firstRuntime.SetHotReload(false);
	secondRuntime.SetHotReload(true);
	if (&firstRuntime == &secondRuntime || !secondRuntime.HotReload()) return 1;
	ly::World firstWorld{ first }, secondWorld{ second };
	ly::EngineContext::Scope scope{ second };
	auto one = firstWorld.SpawnActor<ly::Actor>().lock(), two = secondWorld.SpawnActor<ly::Actor>().lock();
	one->AddComponent<ly::CollisionComponent>();
	two->AddComponent<ly::CollisionComponent>();
	auto& firstScript = one->AddComponent<vega::script::ScriptComponent>("shared.lua");
	auto& secondScript = two->AddComponent<vega::script::ScriptComponent>("shared.lua");
	firstWorld.BeginPlayInternal(); secondWorld.BeginPlayInternal();
	for (int frame = 0; frame < 3; ++frame)
	{
		firstWorld.TickInternal(1.f / 60.f); secondWorld.TickInternal(1.f / 60.f);
	}
	if (!firstScript.LastError().empty() || !secondScript.LastError().empty() || one->GetActorRotation() != 3.f || two->GetActorRotation() != 6.f)
	{
		std::cerr << "Script roots/interpreters leaked across contexts\n";
		return 1;
	}
	if (one->GetCollisionMask() != CollisionLayer::Layer0 || two->GetCollisionMask() != CollisionLayer::Layer7) return 1;
	std::cout << "Optional scripting slots and interleaved worlds are isolated\n";
}
