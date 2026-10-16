#include "editor/ScriptExecution.h"
#include <framework/World.h>
#include <framework/scene/SceneEntityComponent.h>
#include <cmath>

namespace vega
{
	namespace
	{
		EditorScriptParseResult DecodeParent(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			const auto& verb = arguments.tokens[0];
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != (verb == "unparent" ? 2u : 3u) || tokens[1].empty() || (tokens.size() == 3 && tokens[2].empty())) return bad();
			command.text = tokens[1];
			if (tokens.size() == 3) command.property = tokens[2];
			return {command, {}};
		}
		EditorScriptParseResult DecodeExpectWorldLocation(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			const auto real = [&arguments](std::size_t i, float& v) { return arguments.Real(i, v); };
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() < 4 || tokens.size() > 5 || tokens[1].empty() || !real(2, command.location.x) || !real(3, command.location.y) ||
			    (tokens.size() == 5 && (!real(4, command.tolerance) || command.tolerance < 0.f)))
				return bad();
			command.text = tokens[1];
			return {command, {}};
		}
		void ExecuteParent(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& history = execution.history;
			auto& success = execution.success;
			auto& detail = execution.detail;
			const auto find = [&](const std::string& name) -> std::shared_ptr<ly::Actor>
			{
				if (const auto target = context.ResolveGuid(name)) return target;
				for (const auto& weak : context.world->GetActorsByTypeIncludingPending<ly::Actor>())
					if (const auto candidate = weak.lock(); candidate && !candidate->GetIsPendingDestroy())
						if (const auto* identity = candidate->FindComponent<ly::SceneEntityComponent>(); identity && identity->name == name) return candidate;
				return {};
			};
			const auto child = find(command.text), parent = find(command.property);
			if (!child) return;
			if (command.operation == "expect-world-location")
			{
				const auto location = child->GetActorLocation();
				success = std::abs(location.x - command.location.x) <= command.tolerance && std::abs(location.y - command.location.y) <= command.tolerance;
				detail = "actual " + std::to_string(location.x) + " " + std::to_string(location.y);
			}
			else if (command.operation == "expect-parent")
				success = command.property == "none" ? child->GetParent() == nullptr : parent && child->GetParent() == parent.get();
			else if (command.operation == "unparent" || parent)
				success = history.Execute(std::make_unique<ReparentActor>(
				    child->FindComponent<ly::SceneEntityComponent>()->guid, parent ? parent->FindComponent<ly::SceneEntityComponent>()->guid : std::string{}));
		}
	} // namespace
	void RegisterHierarchyScriptCommands(ScriptCommandRegistry& registry)
	{
		registry.Add(
		    {"parent", Schema("child:text parent:text"), "Attach a child actor to a parent by name or GUID.", ScriptMode::Any, DecodeParent, ExecuteParent});
		registry.Add(
		    {"expect-parent", Schema("child:text parent:text"), "Assert an actor parent; none means no parent.", ScriptMode::Any, DecodeParent, ExecuteParent});
		registry.Add({"unparent", Schema("child:text"), "Detach an actor while preserving its world transform.", ScriptMode::Any, DecodeParent, ExecuteParent});
		registry.Add({"expect-world-location", Schema("actor:text x:real y:real [tolerance:real]"),
		    "Assert an actor world location with optional nonnegative tolerance.", ScriptMode::Any, DecodeExpectWorldLocation, ExecuteParent});
	}
} // namespace vega
