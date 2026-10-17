#include "editor/ScriptExecution.h"
#include "editor/EditorFunctionCommands.h"

namespace vega
{
	namespace
	{
		EditorScriptParseResult DecodeCallFunction(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 3 || tokens[1].empty() || tokens[2].empty()) return bad();
			command.text = tokens[1];
			command.property = tokens[2];
			return {command, {}};
		}
		void ExecuteCallFunction(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			auto& detail = execution.detail;
			const auto actors = context.selection.GetAll();
			if (actors.empty()) { detail = "No selected actor"; return; }
			// Multi-select runs the function on every selected actor that exposes it, in selection order.
			std::size_t invoked = 0;
			std::string error;
			for (const auto& actor : actors)
			{
				const auto target = FindPropertyTarget(*actor, command.text);
				if (!target.type || !target.type->FindFunction(command.property)) continue;
				if (!CallReflectedFunction(context, target, actor->GetUniqueID(), command.property, error))
				{
					detail = error;
					execution.state.mPendingPropertyError = error;
					return;
				}
				++invoked;
			}
			if (invoked == 0) { detail = "Unknown target or function: " + command.text + " / " + command.property; return; }
			// A call is not undoable, but it marks the scene dirty so an autosave captures its effects.
			context.SceneChanged();
			success = true;
			detail = "Function called";
		}
	} // namespace
	void RegisterFunctionScriptCommands(ScriptCommandRegistry& registry)
	{
		registry.Add({"call-function", Schema("target:text function:text"),
			"Invoke a reflected CallInEditor function on the selected actor(s); edit mode only unless the function opts into play.", ScriptMode::Any,
			DecodeCallFunction, ExecuteCallFunction});
	}
} // namespace vega
