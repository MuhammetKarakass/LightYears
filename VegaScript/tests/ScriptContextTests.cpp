#include "framework/EngineContext.h"
#include "framework/World.h"
#include "framework/Actor.h"
#include "framework/ProjectSettings.h"
#include "framework/CollisionComponent.h"
#include "framework/reflection/Reflection.h"
#include "script/ScriptRuntime.h"
#include "script/ScriptComponent.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace
{
	class FunctionComponent : public vega::ActorComponent
	{
		VEGA_CLASS(FunctionComponent)
	public:
		using ActorComponent::ActorComponent;
		int pulses = 0;
		void Pulse() { ++pulses; }
		void Fail() { throw std::runtime_error("fixture failure"); }
		void FailUnknown() { throw 42; }
		void Hidden() { pulses = -100; }
	};
	VEGA_REFLECT(FunctionComponent, vega::ActorComponent)
	{
		VEGA_FIELD_READONLY("Pulses", pulses);
		VEGA_FUNCTION(FunctionComponent, Pulse, "Pulse button");
		VEGA_FUNCTION(FunctionComponent, Fail, "Throw exception");
		VEGA_FUNCTION(FunctionComponent, FailUnknown, "Throw unknown exception");
	}
	class FunctionActor : public vega::Actor
	{
		VEGA_CLASS(FunctionActor)
	public:
		using Actor::Actor;
		void Pulse() { SetActorRotation(GetActorRotation() + 10.f); }
	};
	VEGA_REFLECT(FunctionActor, vega::Actor)
	{
		VEGA_FUNCTION(FunctionActor, Pulse, "Actor pulse button");
	}
	class DerivedFunctionActor : public FunctionActor
	{
		VEGA_CLASS(DerivedFunctionActor)
	public:
		using FunctionActor::FunctionActor;
	};
	VEGA_REFLECT(DerivedFunctionActor, FunctionActor)
	{
		(void)type;
	}
	bool ReflectedFunctions(vega::EngineContext& context, const std::filesystem::path& root)
	{
		VEGA_REGISTER_IN(FunctionComponent, context.Types());
		VEGA_REGISTER_IN(FunctionActor, context.Types());
		VEGA_REGISTER_IN(DerivedFunctionActor, context.Types());
		std::ofstream{ root / "first/functions.lua" } << R"(
function on_start(self)
	local component = self.actor:component('FunctionComponent')
	component:call('Pulse')
	assert(component:get('Pulses') == 1)
	self.actor:call('Pulse')
	assert(self.actor:rotation() == 10)
	local function rejected(target, name, reason)
		local ok, message = pcall(function() target:call(name) end)
		assert(not ok and string.find(message, name, 1, true))
		assert(string.find(message, reason, 1, true))
	end
	rejected(component, 'Missing', 'Unknown reflected function')
	rejected(component, 'Hidden', 'Unknown reflected function')
	rejected(component, 'Pulse button', 'Unknown reflected function')
	rejected(self.actor, 'Missing', 'Unknown reflected function')
	rejected(component, 'Fail', 'fixture failure')
	rejected(component, 'FailUnknown', 'unknown C++ exception')
	component:call('Pulse')
	self.actor:destroy()
	rejected(component, 'Pulse', 'no longer alive')
	rejected(self.actor, 'Pulse', 'no longer alive')
end
)";
		vega::World world{ context };
		auto actor = world.SpawnActor<DerivedFunctionActor>().lock();
		auto& component = actor->AddComponent<FunctionComponent>();
		auto& script = actor->AddComponent<vega::script::ScriptComponent>("functions.lua");
		world.BeginPlayInternal();
		// Pending spawns begin their components when the world first ticks.
		world.TickInternal(0.f);
		if (!script.LastError().empty() || component.pulses != 2 || actor->GetActorRotation() != 10.f || !actor->GetIsPendingDestroy())
		{
			std::cerr << "Reflected Lua calls failed: " << script.LastError() << '\n';
			return false;
		}
		return true;
	}
}

int main()
{
	vega::EngineContext first, second;
	vega::ProjectSettings project;
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
	vega::World firstWorld{ first }, secondWorld{ second };
	vega::EngineContext::Scope scope{ second };
	// Only first owns the fixtures, while the ambient context is second.
	if (!ReflectedFunctions(first, root)) return 1;
	auto one = firstWorld.SpawnActor<vega::Actor>().lock(), two = secondWorld.SpawnActor<vega::Actor>().lock();
	one->AddComponent<vega::CollisionComponent>();
	two->AddComponent<vega::CollisionComponent>();
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
