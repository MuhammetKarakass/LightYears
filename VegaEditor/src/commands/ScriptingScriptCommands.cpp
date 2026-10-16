#include "editor/ScriptExecution.h"
#include <script/ScriptComponent.h>
#include "ScriptPropertyValues.h"

namespace vega
{
	namespace
	{
		EditorScriptParseResult DecodeAddScript(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 2 || tokens[1].empty()) return bad();
			command.text = tokens[1];
			return {command, {}};
		}
		EditorScriptParseResult DecodeSetScriptProp(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() < 3 || tokens[1].empty()) return bad();
			command.property = tokens[1];
			command.values.assign(tokens.begin() + 2, tokens.end());
			return {command, {}};
		}
		void ExecuteAddScript(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			auto& detail = execution.detail;
			auto actor = execution.context.selection.Get();
			if (actor && actor->GetWorld() != execution.context.world) actor.reset();
			if (actor && !context.playMode)
			{
				auto& script = actor->AddComponent<script::ScriptComponent>(command.text);
				context.SceneChanged();
				success = script.Prepare();
				detail = success ? "Script attached (not undoable)" : script.LastError();
			}
		}
		void ExecuteExpectScriptError(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& success = execution.success;
			auto& detail = execution.detail;
			auto actor = execution.context.selection.Get();
			if (actor && actor->GetWorld() != execution.context.world) actor.reset();
			if (auto* script = actor ? actor->FindComponent<script::ScriptComponent>() : nullptr)
			{
				success = command.text == "none" ? script->LastError().empty() : script->LastError().find(command.text) != std::string::npos;
				detail = script->LastError();
			}
		}
		void ExecuteSetScriptProp(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& history = execution.history;
			auto& success = execution.success;
			auto& detail = execution.detail;
			auto actor = execution.context.selection.Get();
			if (actor && actor->GetWorld() != execution.context.world) actor.reset();
			const auto target = actor ? FindPropertyTarget(*actor, "ScriptComponent") : ReflectedTarget{};
			auto* script = dynamic_cast<script::ScriptComponent*>(static_cast<ly::ActorComponent*>(target.object));
			if (!script)
			{
				detail = "No ScriptComponent";
				return;
			}
			const auto entry = script->Properties().find(command.property);
			if (entry == script->Properties().end())
			{
				detail = "Unknown script property";
				return;
			}
			ly::reflect::PropertyInfo property;
			property.kind = ly::reflect::KindOf(entry->second);
			ly::reflect::PropertyValue value;
			float tolerance = command.tolerance;
			const bool expect = command.operation == "expect-script-prop";
			if (!ParsePropertyValue(property, command.values, expect, value, tolerance)) detail = "Invalid script property value or tolerance";
			else if (expect) success = PropertyEquals(entry->second, value, tolerance);
			else
				success = history.Execute(std::make_unique<SetScriptPropertyCommand>(actor->GetUniqueID(), *target.componentIdentity, command.property, value));
		}
	} // namespace
	void RegisterScriptingScriptCommands(ScriptCommandRegistry& registry)
	{
		registry.Add({"add-script", Schema("path:text"), "Attach and prepare a Lua script on the selected actor in Edit mode.", ScriptMode::Edit,
		    DecodeAddScript, ExecuteAddScript});
		registry.Add({"expect-script-error", Schema("message:text"), "Assert a script error contains a message; none means no error.", ScriptMode::Any,
		    DecodeAddScript, ExecuteExpectScriptError});
		registry.Add({"set-script-prop", Schema("property:text value:value..."), "Set a script property using its declared type.", ScriptMode::Any,
		    DecodeSetScriptProp, ExecuteSetScriptProp});
		registry.Add({"expect-script-prop", Schema("property:text value:value..."), "Assert a script property, optionally followed by nonnegative tolerance.",
		    ScriptMode::Any, DecodeSetScriptProp, ExecuteSetScriptProp});
	}
} // namespace vega
