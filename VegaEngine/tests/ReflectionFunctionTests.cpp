#include "framework/EngineContext.h"
#include "framework/Actor.h"
#include "framework/ActorComponent.h"
#include "framework/World.h"
#include "framework/reflection/Reflection.h"
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
	int failures = 0;
	void Check(bool condition, const char* message)
	{
		if (!condition) { std::cerr << message << '\n'; ++failures; }
	}

	// A minimal component whose CallInEditor-style functions mutate a reflected field.
	class ActionComponent : public vega::ActorComponent
	{
	public:
		using ActorComponent::ActorComponent;
		void Bump() { ++mCount; }
		void Fail() { throw std::runtime_error("action refused"); }
		int GetCount() const { return mCount; }
	private:
		int mCount = 0;
	};

	class DerivedActionComponent : public ActionComponent
	{
	public:
		using ActionComponent::ActionComponent;
		void Reset() { mBase = 0; }
		int mBase = 0;
	};

	void TestFunctionMetadata()
	{
		vega::reflect::Class<ActionComponent, vega::ActorComponent> builder{ "ActionComponent" };
		builder.Property("Count", [](const ActionComponent& c) { return c.GetCount(); });
		builder.Function("Bump", "Bump", &ActionComponent::Bump);
		builder.Function("Fail", "Force failure", &ActionComponent::Fail).Tooltip("Always throws");
		builder.Function("PlayToo", "Bump in play", &ActionComponent::Bump).CallableInPlay();
		const auto* type = vega::reflect::CurrentTypeRegistry().Find("ActionComponent");
		Check(type && type->FindFunction("Bump"), "Function must be registered on the type");
		const auto* bump = type->FindFunction("Bump");
		Check(bump && bump->name == "Bump" && bump->label == "Bump" && bump->tooltip.empty() && !bump->callableInPlay,
			"Function name, label and defaults must be stored");
		const auto* fail = type->FindFunction("Fail");
		Check(fail && fail->label == "Force failure" && fail->tooltip == "Always throws" && !fail->callableInPlay,
			"Function label and tooltip must be stored");
		Check(type->FindFunction("PlayToo")->callableInPlay, "CallableInPlay must be recorded");
		Check(!type->FindFunction("Missing"), "Unknown function must not resolve");
		Check(type->AllFunctions().size() == 3, "AllFunctions must list every local function in order");
	}

	void TestFunctionInvocation()
	{
		vega::World world{ nullptr };
		auto actor = world.SpawnActor<vega::Actor>().lock();
		auto& component = actor->AddComponent<ActionComponent>();
		const auto* type = vega::reflect::CurrentTypeRegistry().Find("ActionComponent");
		// Erased invokers receive the registered root pointer, as property access does.
		auto* root = static_cast<vega::ActorComponent*>(&component);
		type->FindFunction("Bump")->invoke(root);
		type->FindFunction("Bump")->invoke(root);
		Check(component.GetCount() == 2, "Reflected function must run on the live object");
		bool threw = false;
		try { type->FindFunction("Fail")->invoke(root); }
		catch (const std::runtime_error& error) { threw = std::string{ error.what() } == "action refused"; }
		Check(threw, "The invoker must let a function's exception propagate to the caller");
	}

	void TestInheritedFunctions()
	{
		vega::reflect::Class<DerivedActionComponent, ActionComponent> builder{ "DerivedActionComponent" };
		builder.Function("Reset", "Reset", &DerivedActionComponent::Reset);
		const auto* type = vega::reflect::CurrentTypeRegistry().Find("DerivedActionComponent");
		Check(type && type->FindFunction("Bump") && type->FindFunction("Reset"), "Derived types must inherit base functions");
		Check(type->AllFunctions().size() == 4, "AllFunctions must visit inherited functions before local ones");
		Check(type->AllFunctions().front()->name == "Bump" && type->AllFunctions().back()->name == "Reset",
			"Inherited functions must come before local functions");
	}
}

int main()
{
	vega::EngineContext engineContext;
	vega::EngineContext::Scope contextScope{ engineContext };
	TestFunctionMetadata();
	TestFunctionInvocation();
	TestInheritedFunctions();
	if (failures) return 1;
	std::cout << "Reflection function tests passed\n";
	return 0;
}
