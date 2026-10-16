#include "editor/ScriptExecution.h"

namespace vega
{
	namespace
	{
		EditorScriptParseResult DecodeAutosaveNow(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			const auto& verb = arguments.tokens[0];
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };

			const bool noPath = verb == "autosave-now" || verb == "simulate-unclean-exit";
			const bool expect = verb == "expect-autosave" || verb == "expect-recovery";
			if (tokens.size() != (noPath ? 1u : 2u) && !(expect && tokens.size() == 3)) return bad();
			if (!noPath && tokens[1].empty()) return bad();
			if (tokens.size() == 3 && tokens[2] != "true" && tokens[2] != "false") return bad();
			if (!noPath) command.text = tokens[1];
			command.overwrite = tokens.size() != 3 || tokens[2] == "true";
			return {command, {}};
		}
		void ExecuteAutosaveNow(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			auto& detail = execution.detail;
			success = context.autosaveCommand && context.autosaveCommand(command, detail);
		}
	} // namespace
	void RegisterAutosaveScriptCommands(ScriptCommandRegistry& registry)
	{
		registry.Add({"autosave-now", Schema(""), "Write a recovery snapshot of the active scene.", ScriptMode::Edit, DecodeAutosaveNow, ExecuteAutosaveNow});
		registry.Add({"simulate-unclean-exit", Schema(""), "Leave the autosave session marked unclean for recovery testing.", ScriptMode::Edit,
		    DecodeAutosaveNow, ExecuteAutosaveNow});
		registry.Add(
		    {"recover-autosave", Schema("path:text"), "Restore a scene from its recovery snapshot.", ScriptMode::Edit, DecodeAutosaveNow, ExecuteAutosaveNow});
		registry.Add({"expect-autosave", Schema("path:text [exists:true|false]"), "Assert whether a scene has an autosave snapshot (defaults to true).",
		    ScriptMode::Any, DecodeAutosaveNow, ExecuteAutosaveNow});
		registry.Add({"expect-recovery", Schema("path:text [exists:true|false]"), "Assert whether a scene is offered for recovery (defaults to true).",
		    ScriptMode::Any, DecodeAutosaveNow, ExecuteAutosaveNow});
	}
} // namespace vega
