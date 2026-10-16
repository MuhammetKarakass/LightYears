#include "editor/ScriptExecution.h"

namespace vega
{
	void RegisterAssetScriptCommands(ScriptCommandRegistry&);
	void RegisterAutosaveScriptCommands(ScriptCommandRegistry&);
	void RegisterPropertyScriptCommands(ScriptCommandRegistry&);
	void RegisterHierarchyScriptCommands(ScriptCommandRegistry&);
	void RegisterContentScriptCommands(ScriptCommandRegistry&);
	void RegisterSelectionScriptCommands(ScriptCommandRegistry&);
	void RegisterSettingsScriptCommands(ScriptCommandRegistry&);
	void RegisterConsoleScriptCommands(ScriptCommandRegistry&);
	void RegisterProfilerScriptCommands(ScriptCommandRegistry&);
	void RegisterPrefabScriptCommands(ScriptCommandRegistry&);
	void RegisterSceneScriptCommands(ScriptCommandRegistry&);
	void RegisterProjectScriptCommands(ScriptCommandRegistry&);
	void RegisterViewScriptCommands(ScriptCommandRegistry&);
	void RegisterScriptingScriptCommands(ScriptCommandRegistry&);
	void RegisterSessionScriptCommands(ScriptCommandRegistry&);
	void RegisterJsonScriptCommands(ScriptCommandRegistry&);
	void RegisterExportScriptCommands(ScriptCommandRegistry&);
	void RegisterCameraScriptCommands(ScriptCommandRegistry&);
	void RegisterCoreScriptCommands(ScriptCommandRegistry& registry)
	{
		RegisterAssetScriptCommands(registry);
		RegisterAutosaveScriptCommands(registry);
		RegisterPropertyScriptCommands(registry);
		RegisterHierarchyScriptCommands(registry);
		RegisterContentScriptCommands(registry);
		RegisterSelectionScriptCommands(registry);
		RegisterSettingsScriptCommands(registry);
		RegisterConsoleScriptCommands(registry);
		RegisterProfilerScriptCommands(registry);
		RegisterPrefabScriptCommands(registry);
		RegisterSceneScriptCommands(registry);
		RegisterProjectScriptCommands(registry);
		RegisterViewScriptCommands(registry);
		RegisterScriptingScriptCommands(registry);
		RegisterSessionScriptCommands(registry);
		RegisterJsonScriptCommands(registry);
		RegisterExportScriptCommands(registry);
		RegisterCameraScriptCommands(registry);
		registry.Add({"help", Schema("[command:text]"), "List registered commands or describe one command.", ScriptMode::Any,
		    [](const ScriptArguments& arguments) -> EditorScriptParseResult
		    {
			    EditorScriptCommand command;
			    command.operation = "help";
			    if (arguments.tokens.size() == 2) command.text = arguments.tokens[1];
			    return {command, {}};
		    },
		    [](ScriptExecution& execution, const EditorScriptCommand& command)
		    {
			    execution.success = command.text.empty() || execution.registry.Find(command.text);
			    execution.detail = execution.registry.Help(command.text);
		    }});
	}
} // namespace vega
