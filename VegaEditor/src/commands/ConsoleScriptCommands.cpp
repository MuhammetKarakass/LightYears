#include "editor/ScriptExecution.h"
#include <algorithm>

namespace vega
{
	namespace
	{
		EditorScriptParseResult DecodeExpectConsole(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 3 || tokens[2].empty() || (tokens[1] != "Info" && tokens[1] != "Warning" && tokens[1] != "Error")) return bad();
			command.property = tokens[1];
			command.text = tokens[2];
			return {command, {}};
		}
		EditorScriptParseResult DecodeExpectNoErrors(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 1) return bad();
			return {command, {}};
		}
		void ExecuteExpectConsole(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			if (context.console)
			{
				const auto records = context.console->Snapshot();
				if (command.operation == "expect-no-errors")
					success = std::none_of(records.begin(), records.end(), [](const auto& r) { return r.level >= vega::debug::LogLevel::Error; });
				else
					success = std::any_of(records.begin(), records.end(),
					    [&](const auto& r)
					    {
						    const auto level = command.property == "Info" ? vega::debug::LogLevel::Info
						        : command.property == "Warning"           ? vega::debug::LogLevel::Warning
						                                                  : vega::debug::LogLevel::Error;
						    return (r.level == level || (level == vega::debug::LogLevel::Error && r.level == vega::debug::LogLevel::Fatal)) &&
						        r.message.find(command.text) != std::string::npos;
					    });
			}
		}
		void ExecuteClearConsole(ScriptExecution& execution, const EditorScriptCommand&)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			if (context.clearConsole)
			{
				context.clearConsole();
				success = true;
			}
			else if (context.console)
			{
				context.console->Clear();
				success = true;
			}
		}
	} // namespace
	void RegisterConsoleScriptCommands(ScriptCommandRegistry& registry)
	{
		registry.Add({"expect-console", Schema("level:Info|Warning|Error message:text"), "Assert that Console contains a message at the requested severity.",
		    ScriptMode::Any, DecodeExpectConsole, ExecuteExpectConsole});
		registry.Add({"expect-no-errors", Schema(""), "Assert that Console has no error or fatal messages.", ScriptMode::Any, DecodeExpectNoErrors,
		    ExecuteExpectConsole});
		registry.Add({"clear-console", Schema(""), "Clear captured Console messages.", ScriptMode::Any, DecodeExpectNoErrors, ExecuteClearConsole});
	}
} // namespace vega
