#include "editor/ScriptExecution.h"
#include "editor/JsonDocument.h"
#include <framework/ProjectSettings.h>
#include <filesystem>

namespace vega
{
	namespace
	{
		EditorScriptParseResult DecodeSetProjectStartup(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 2) return bad();
			command.text = tokens[1];
			return {command, {}};
		}
		void ExecuteSetProjectStartup(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			auto& detail = execution.detail;
			try
			{
				if (!context.document) throw std::runtime_error("No project asset root");
				const auto path = context.document->Root() / "vega.project.json";
				auto project = std::filesystem::exists(path) ? vega::ProjectSettings::Load(path) : vega::ProjectSettings{};
				if (command.operation == "expect-project-startup") success = project.startupScene == command.text;
				else
				{
					project.startupScene = command.text;
					project = vega::ProjectSettings::Parse(project.ToJson());
					success = AtomicWriteText(path, project.ToJson().dump(2) + "\n", detail);
				}
			}
			catch (const std::exception& error)
			{
				detail = error.what();
			}
		}
	} // namespace
	void RegisterProjectScriptCommands(ScriptCommandRegistry& registry)
	{
		registry.Add({"set-project-startup", Schema("path:string"), "Set the project startup scene path.", ScriptMode::Any, DecodeSetProjectStartup,
		    ExecuteSetProjectStartup});
		registry.Add({"expect-project-startup", Schema("path:string"), "Assert the project startup scene path.", ScriptMode::Any, DecodeSetProjectStartup,
		    ExecuteSetProjectStartup});
	}
} // namespace vega
