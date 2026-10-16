#include "editor/ScriptExecution.h"

namespace vega
{
	namespace
	{
		EditorScriptParseResult DecodeAssetMove(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			const auto& verb = arguments.tokens[0];
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 3 || tokens[1].empty() || tokens[2].empty()) return bad();
			if (verb == "expect-asset" && tokens[2] != "true" && tokens[2] != "false") return bad();
			if (verb == "asset-delete" && tokens[2] != "confirm") return bad();
			command.text = tokens[1];
			command.values = {tokens[2]};
			return {command, {}};
		}
		void ExecuteAssetMove(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			auto& detail = execution.detail;
			success = context.assetCommand && context.assetCommand(command, detail);
		}
	} // namespace
	void RegisterAssetScriptCommands(ScriptCommandRegistry& registry)
	{
		registry.Add({"asset-move", Schema("source:text destination:text"), "Move an asset and update references.", ScriptMode::Edit, DecodeAssetMove,
		    ExecuteAssetMove});
		registry.Add(
		    {"asset-rename", Schema("path:text name:text"), "Rename an asset and update references.", ScriptMode::Edit, DecodeAssetMove, ExecuteAssetMove});
		registry.Add({"asset-delete", Schema("path:text confirmation:confirm"), "Delete an unreferenced asset after confirmation.", ScriptMode::Edit,
		    DecodeAssetMove, ExecuteAssetMove});
		registry.Add(
		    {"expect-asset", Schema("path:text exists:true|false"), "Assert whether an asset exists.", ScriptMode::Any, DecodeAssetMove, ExecuteAssetMove});
	}
} // namespace vega
