#include "editor/ScriptExecution.h"
#include <framework/World.h>

namespace vega
{
	namespace
	{
		EditorScriptParseResult DecodeFocus(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 1) return bad();
			return {command, {}};
		}
		EditorScriptParseResult DecodeExpectVisible(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 1) return bad();
			return {command, {}};
		}
		EditorScriptParseResult DecodeCamera(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 2 || (tokens[1] != "editor" && tokens[1] != "game")) return bad();
			command.text = tokens[1];
			return {command, {}};
		}
		EditorScriptParseResult DecodeZoom(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			const auto real = [&arguments](std::size_t i, float& v) { return arguments.Real(i, v); };
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 2 || !real(1, command.factor) || command.factor <= 0.f) return bad();
			return {command, {}};
		}
		EditorScriptParseResult DecodePan(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			const auto real = [&arguments](std::size_t i, float& v) { return arguments.Real(i, v); };
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 3 || !real(1, command.location.x) || !real(2, command.location.y)) return bad();
			return {command, {}};
		}
		void ExecuteFocus(ScriptExecution& execution, const EditorScriptCommand&)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			success = context.FocusSelection();
		}
		void ExecuteExpectVisible(ScriptExecution& execution, const EditorScriptCommand&)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			auto actor = execution.context.selection.Get();
			if (actor && actor->GetWorld() != execution.context.world) actor.reset();
			const auto view = context.world->GetWorldView();
			success =
			    actor && actor->GetActorGlobalBounds().findIntersection(sf::FloatRect{view.getCenter() - view.getSize() * 0.5f, view.getSize()}).has_value();
		}
		void ExecuteCamera(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			success = context.SetEditorCamera(command.text == "editor");
		}
		void ExecuteExpectCamera(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			success = context.world->GetViewOverride().has_value() == (command.text == "editor");
		}
		void ExecuteZoom(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			if (context.SetEditorCamera(true))
			{
				if (command.operation == "zoom") context.camera->ZoomAt(command.factor, context.world->GetWorldView().getCenter());
				else context.camera->Pan(command.location);
				context.ApplyCamera();
				success = true;
			}
		}
	} // namespace
	void RegisterCameraScriptCommands(ScriptCommandRegistry& registry)
	{
		registry.Add({"focus", Schema(""), "Focus the editor camera on the selection.", ScriptMode::Any, DecodeFocus, ExecuteFocus});
		registry.Add({"expect-visible", Schema(""), "Assert the selected actor intersects the current world view.", ScriptMode::Any, DecodeExpectVisible,
		    ExecuteExpectVisible});
		registry.Add({"camera", Schema("camera:editor|game"), "Choose the editor or game camera.", ScriptMode::Any, DecodeCamera, ExecuteCamera});
		registry.Add({"expect-camera", Schema("camera:editor|game"), "Assert the active camera kind.", ScriptMode::Any, DecodeCamera, ExecuteExpectCamera});
		registry.Add({"zoom", Schema("factor:real"), "Zoom the editor camera by a strictly positive finite factor.", ScriptMode::Any, DecodeZoom, ExecuteZoom});
		registry.Add({"pan", Schema("x:real y:real"), "Pan the editor camera by a world-coordinate offset.", ScriptMode::Any, DecodePan, ExecuteZoom});
	}
} // namespace vega
