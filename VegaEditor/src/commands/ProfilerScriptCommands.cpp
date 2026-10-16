#include "editor/ScriptExecution.h"
#include <framework/debug/Profiler.h>
#include <filesystem>

namespace vega
{
	namespace
	{
		EditorScriptParseResult DecodeProfiler(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			const auto& verb = arguments.tokens[0];
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 2 || tokens[1].empty()) return bad();
			if ((verb == "profiler" || verb == "diagnostics") && tokens[1] != "on" && tokens[1] != "off") return bad();
			command.text = tokens[1];
			return {command, {}};
		}
		void ExecuteProfiler(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& success = execution.success;
			ly::debug::Profiler::SetEnabled(command.text == "on");
			success = ly::debug::Profiler::IsEnabled() == (command.text == "on");
		}
		void ExecuteDiagnostics(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			if (context.diagnosticsVisible)
			{
				context.diagnosticsVisible(command.text == "on");
				success = true;
			}
		}
		void ExecuteExpectProfileScope(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& success = execution.success;
			for (const auto& frame : ly::debug::Profiler::CaptureFrames())
				for (const auto& scope : frame.scopes)
					if (scope.name == command.text) success = true;
		}
		void ExecuteCaptureProfile(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& success = execution.success;
			auto& detail = execution.detail;
			auto& mExportLayout = execution.state.mExportLayout;
			try
			{
				const auto requested = std::filesystem::u8path(command.text);
				const auto path = std::filesystem::weakly_canonical(requested.is_absolute() ? requested : mExportLayout.generatedRoot / "Saved" / requested);
				const auto inside = [](const auto& file, const auto& root)
				{
					if (root.empty()) return false;
					const auto relative = file.lexically_relative(std::filesystem::weakly_canonical(root));
					return !relative.empty() && *relative.begin() != "..";
				};
				if (inside(path, mExportLayout.assetRoot) || inside(path, mExportLayout.sourceAssetRoot) || inside(path, mExportLayout.visualAssetsDir))
					detail = "Profile captures cannot be written into assets";
				else success = ly::debug::Profiler::CaptureToFile(path, detail);
			}
			catch (const std::exception& error)
			{
				detail = error.what();
			}
		}
	} // namespace
	void RegisterProfilerScriptCommands(ScriptCommandRegistry& registry)
	{
		registry.Add({"profiler", Schema("visible:on|off"), "Enable or disable profiler recording.", ScriptMode::Any, DecodeProfiler, ExecuteProfiler});
		registry.Add(
		    {"diagnostics", Schema("visible:on|off"), "Show or hide Console and Profiler panels.", ScriptMode::Any, DecodeProfiler, ExecuteDiagnostics});
		registry.Add({"expect-profile-scope", Schema("name:text"), "Assert that a captured profiler frame contains a scope.", ScriptMode::Any, DecodeProfiler,
		    ExecuteExpectProfileScope});
		registry.Add({"capture-profile", Schema("name:text"), "Save captured profiler frames outside asset roots.", ScriptMode::Any, DecodeProfiler,
		    ExecuteCaptureProfile});
	}
} // namespace vega
