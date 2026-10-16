#include "editor/ScriptExecution.h"
#include <filesystem>

namespace vega
{
	namespace
	{
		EditorScriptParseResult DecodeExport(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() < 2 || tokens.size() > 3 || tokens[1].empty() || (tokens.size() == 3 && tokens[2] != "overwrite")) return bad();
			command.text = tokens[1];
			command.overwrite = tokens.size() == 3;
			return {command, {}};
		}
		EditorScriptParseResult DecodeExpectExportOk(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 1) return bad();
			return {command, {}};
		}
		void ExecuteExport(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& line = execution.line;
			auto& detail = execution.detail;
			auto& mExportFuture = execution.state.mExportFuture;
			auto& mExportLayout = execution.state.mExportLayout;
			auto& mExportLine = execution.state.mExportLine;
			// The script always runs the smoke test, so expect-export-ok can check the whole result.
			GameExportRequest request;
			request.outputDir = std::filesystem::u8path(command.text);
			request.overwrite = command.overwrite;
			request.runSmokeTest = true;
			try
			{
				mExportFuture = ExportGameAsync(mExportLayout, request);
				mExportLine = line;
				execution.deferReport = true;
				return;
			}
			catch (const std::exception& error)
			{
				detail = error.what();
			}
		}
		void ExecuteExpectExportOk(ScriptExecution& execution, const EditorScriptCommand&)
		{
			auto& success = execution.success;
			auto& detail = execution.detail;
			auto& mLastExport = execution.state.mLastExport;
			success = mLastExport && mLastExport->success && mLastExport->smokeTestRan && mLastExport->smokeTestPassed;
			detail = mLastExport ? DescribeGameExport(*mLastExport) : std::string{"No export has run"};
		}
	} // namespace
	void RegisterExportScriptCommands(ScriptCommandRegistry& registry)
	{
		registry.Add({"export", Schema("path:text [confirmation:overwrite]"), "Export the game asynchronously and run its smoke test.", ScriptMode::Any,
		    DecodeExport, ExecuteExport});
		registry.Add({"expect-export-ok", Schema(""), "Assert the previous export and its smoke test succeeded.", ScriptMode::Any, DecodeExpectExportOk,
		    ExecuteExpectExportOk});
	}
} // namespace vega
