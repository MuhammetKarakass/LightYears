#include "framework/EngineContext.h"
#include "framework/Actor.h"
#include "framework/ActorComponent.h"
#include "framework/World.h"
#include "framework/reflection/Reflection.h"
#include "framework/scene/SceneEntityComponent.h"
#include "editor/EditorCommands.h"
#include "editor/EditorFunctionCommands.h"
#include "editor/EditorScript.h"
#include "editor/ScriptExecution.h"
#include "editor/ScriptCommandRegistry.h"
#include <SFML/Graphics/RenderWindow.hpp>
#include <iostream>
#include <stdexcept>
#include <string>

namespace vega
{
	void RegisterFunctionScriptCommands(ScriptCommandRegistry&);
}

namespace
{
	int failures = 0;
	void Check(bool condition, const char* message)
	{
		if (!condition) { std::cerr << message << '\n'; ++failures; }
	}

	// Test-only component whose reflected functions mutate a field, throw, or opt into play mode.
	class TestActionComponent : public vega::ActorComponent
	{
	public:
		using ActorComponent::ActorComponent;
		void Count() { ++mCount; }
		void Throw() { throw std::runtime_error("boom"); }
		int GetCount() const { return mCount; }
	private:
		int mCount = 0;
	};

	void RegisterTestTypes()
	{
		vega::reflect::RegisterEngineTypes();
		vega::reflect::Class<TestActionComponent, vega::ActorComponent> builder{ "TestActionComponent" };
		builder.Property("Count", [](const TestActionComponent& c) { return c.GetCount(); });
		builder.Function("Count", "Count", &TestActionComponent::Count);
		builder.Function("Throw", "Throw", &TestActionComponent::Throw);
		builder.Function("PlayCount", "Play Count", &TestActionComponent::Count).CallableInPlay();
	}

	// Runs one parsed line through the real command decoder and handler, the path a .vegascript takes.
	bool Run(vega::EditorContext& context, vega::CommandHistory& history, vega::ScriptCommandRegistry& registry,
		const std::string& line, std::string& detail)
	{
		const auto parsed = vega::ParseEditorScriptLine(line, registry);
		if (!parsed.command) { detail = parsed.error; return false; }
		sf::RenderWindow window;
		vega::EditorScriptState state;
		vega::ScriptExecution execution{ context, history, window, state, registry, line };
		const auto* definition = registry.Find(parsed.command->operation);
		if (!definition) { detail = "Unknown command"; return false; }
		definition->execute(execution, *parsed.command);
		detail = execution.detail;
		return execution.success;
	}
}

int main()
{
	vega::EngineContext engineContext;
	vega::EngineContext::Scope contextScope{ engineContext };
	RegisterTestTypes();
	vega::World world{ nullptr };
	auto first = world.SpawnActor<vega::Actor>().lock();
	first->AddComponent<vega::SceneEntityComponent>("a", "Caller A");
	auto& firstComponent = first->AddComponent<TestActionComponent>();
	auto second = world.SpawnActor<vega::Actor>().lock();
	second->AddComponent<vega::SceneEntityComponent>("b", "Caller B");
	auto& secondComponent = second->AddComponent<TestActionComponent>();
	world.TickInternal(0.f);
	vega::EditorSelection selection;
	vega::EditorContext context{ &world, selection };
	vega::CommandHistory history{ context };
	vega::ScriptCommandRegistry registry;
	vega::RegisterFunctionScriptCommands(registry);

	std::string detail;
	selection.Select(first);
	Check(Run(context, history, registry, "call-function TestActionComponent Count", detail), "single call must succeed");
	Check(firstComponent.GetCount() == 1, "single call must run on the selected object");
	Check(context.sceneRevision == 1, "a successful call must mark the scene dirty");

	selection.Add(second);
	const auto revision = context.sceneRevision;
	Check(Run(context, history, registry, "call-function TestActionComponent Count", detail), "multi call must succeed");
	Check(firstComponent.GetCount() == 2 && secondComponent.GetCount() == 1, "multi-select must invoke each selected actor");
	Check(context.sceneRevision == revision + 1, "one gesture must advance the edit revision once");

	const auto beforeThrow = firstComponent.GetCount();
	Check(!Run(context, history, registry, "call-function TestActionComponent Throw", detail), "a throwing function must report failure, not crash");
	Check(detail == "boom", "the exception message must be reported");
	Check(context.propertyError == "boom" && context.propertyErrorName == "Throw", "the failure must be recorded like a refused property write");
	Check(firstComponent.GetCount() == beforeThrow, "a failed call must not mutate");
	Check(!Run(context, history, registry, "call-function TestActionComponent Missing", detail), "an unknown function must fail");
	Check(detail.find("Unknown") != std::string::npos, "an unknown function must be explained");

	context.playMode = true;
	Check(!Run(context, history, registry, "call-function TestActionComponent Count", detail), "an edit-only function must be refused while playing");
	Check(detail.find("play mode") != std::string::npos, "the play refusal must explain itself");
	Check(Run(context, history, registry, "call-function TestActionComponent PlayCount", detail), "a play-callable function must run while playing");
	Check(firstComponent.GetCount() == 3 && secondComponent.GetCount() == 2, "a play call must reach every selected actor");
	context.playMode = false;

	if (failures) return 1;
	std::cout << "Editor function tests passed\n";
	return 0;
}
