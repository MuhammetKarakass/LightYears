#include "editor/ScriptExecution.h"

namespace vega
{
	namespace
	{
		EditorScriptParseResult DecodeTool(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			const auto& verb = arguments.tokens[0];
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 2) return bad();
			if (verb == "tool")
			{
				if (tokens[1] != "move" && tokens[1] != "rotate") return bad();
			}
			else if (verb == "content-view")
			{
				if (tokens[1] != "grid" && tokens[1] != "list") return bad();
			}
			else
			{
				if (tokens[1] != "on" && tokens[1] != "off") return bad();
			}
			command.text = tokens[1];
			return {command, {}};
		}
		void ExecuteTool(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			context.viewSettings.tool = command.text == "move" ? EditorTool::Move : EditorTool::Rotate;
			success = true;
		}
		void ExecuteSnap(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			context.viewSettings.snap = command.text == "on";
			success = true;
		}
		void ExecuteGrid(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			context.viewSettings.grid = command.text == "on";
			success = true;
		}
		void ExecuteColliders(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			context.viewSettings.colliders = command.text == "on";
			success = true;
		}
		void ExecuteContentView(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			context.viewSettings.contentGrid = command.text == "grid";
			context.viewSettings.requestContent = true;
			success = true;
		}
	} // namespace
	void RegisterViewScriptCommands(ScriptCommandRegistry& registry)
	{
		registry.Add({"tool", Schema("tool:move|rotate"), "Choose the transform gizmo tool.", ScriptMode::Any, DecodeTool, ExecuteTool});
		registry.Add({"snap", Schema("enabled:on|off"), "Enable or disable transform snapping.", ScriptMode::Any, DecodeTool, ExecuteSnap});
		registry.Add({"grid", Schema("enabled:on|off"), "Show or hide the viewport grid.", ScriptMode::Any, DecodeTool, ExecuteGrid});
		registry.Add({"colliders", Schema("enabled:on|off"), "Show or hide collider overlays.", ScriptMode::Any, DecodeTool, ExecuteColliders});
		registry.Add(
		    {"content-view", Schema("view:grid|list"), "Choose the Content panel grid or list view.", ScriptMode::Any, DecodeTool, ExecuteContentView});
	}
} // namespace vega
