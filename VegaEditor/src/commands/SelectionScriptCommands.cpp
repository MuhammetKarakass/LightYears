#include "editor/ScriptExecution.h"
#include <framework/World.h>
#include <framework/scene/SceneEntityComponent.h>
#include <charconv>

namespace vega
{
	namespace
	{
		EditorScriptParseResult DecodeSelectAdd(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 2 || tokens[1].empty()) return bad();
			command.text = tokens[1];
			return {command, {}};
		}
		EditorScriptParseResult DecodeSelectAll(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 1) return bad();
			return {command, {}};
		}
		EditorScriptParseResult DecodeExpectSelectionCount(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			const auto integer = [&arguments](std::size_t i, unsigned int& v) { return arguments.Integer(i, v); };
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 2 || !integer(1, command.number)) return bad();
			return {command, {}};
		}
		EditorScriptParseResult DecodeSelectRect(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			const auto real = [&arguments](std::size_t i, float& v) { return arguments.Real(i, v); };
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 5 || !real(1, command.location.x) || !real(2, command.location.y) || !real(3, command.rectangleEnd.x) ||
			    !real(4, command.rectangleEnd.y))
				return bad();
			return {command, {}};
		}
		EditorScriptParseResult DecodeMoveSelection(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			const auto& verb = arguments.tokens[0];
			const auto real = [&arguments](std::size_t i, float& v) { return arguments.Real(i, v); };
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };

			if (verb == "paste" && tokens.size() == 1) command.number = 0;
			else
			{
				if (tokens.size() != 3 || !real(1, command.location.x) || !real(2, command.location.y)) return bad();
				command.number = 1;
			}
			return {command, {}};
		}
		EditorScriptParseResult DecodeSelectType(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			const auto integer = [&arguments](std::size_t i, unsigned int& v) { return arguments.Integer(i, v); };
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() < 2 || tokens.size() > 3 || (tokens.size() == 3 && !integer(2, command.number))) return bad();
			command.text = tokens[1];
			return {command, {}};
		}
		void ExecuteSelectAdd(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			std::shared_ptr<ly::Actor> target;
			unsigned int id = 0;
			const auto number = std::from_chars(command.text.data(), command.text.data() + command.text.size(), id);
			for (const auto& weak : context.world->GetActorsByTypeIncludingPending<ly::Actor>())
			{
				const auto candidate = weak.lock();
				if (!candidate || candidate->GetIsPendingDestroy()) continue;
				const auto* identity = candidate->FindComponent<ly::SceneEntityComponent>();
				if ((identity && (identity->name == command.text || identity->guid == command.text)) ||
				    (number.ec == std::errc{} && number.ptr == command.text.data() + command.text.size() && candidate->GetUniqueID() == id))
				{
					target = candidate;
					break;
				}
			}
			if (target)
			{
				if (command.operation == "select-add") context.selection.Add(target);
				else context.selection.Remove(target);
				success = true;
			}
		}
		void ExecuteSelectAll(ScriptExecution& execution, const EditorScriptCommand&)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			context.selection.Clear();
			for (const auto& weak : context.world->GetActorsByTypeIncludingPending<ly::Actor>()) context.selection.Add(weak);
			success = true;
		}
		void ExecuteCopy(ScriptExecution& execution, const EditorScriptCommand&)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			success = CopySelection(context);
		}
		void ExecuteExpectSelectionCount(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			auto& detail = execution.detail;
			success = context.selection.Count() == command.number;
			detail = "selection count " + std::to_string(context.selection.Count()) + " expected " + std::to_string(command.number);
		}
		void ExecuteSelectRect(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			SelectRectangle(context, command.location, command.rectangleEnd);
			success = true;
		}
		void ExecuteMoveSelection(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& history = execution.history;
			auto& success = execution.success;
			success = TransformSelection(context, history, CaptureSelectionTransform(context.selection), context.selection.Centre(), command.location);
		}
		void ExecutePaste(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& history = execution.history;
			auto& success = execution.success;
			success = PasteSelection(context, history, command.number ? command.location : context.world->GetWorldView().getCenter());
		}
		void ExecuteSelectType(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			unsigned int index = 0;
			for (const auto& weak : context.world->GetActorsByTypeIncludingPending<ly::Actor>())
			{
				if (auto candidate = weak.lock(); candidate && !candidate->GetIsPendingDestroy() && ActorTypeName(*candidate) == command.text)
				{
					if (index++ == command.number)
					{
						success = SelectActor{candidate->GetUniqueID()}.Execute(context);
						break;
					}
				}
			}
		}
	} // namespace
	void RegisterSelectionScriptCommands(ScriptCommandRegistry& registry)
	{
		registry.Add({"select-add", Schema("actor:text"), "Add an actor to the selection by name, GUID or numeric ID.", ScriptMode::Any, DecodeSelectAdd,
		    ExecuteSelectAdd});
		registry.Add({"select-remove", Schema("actor:text"), "Remove an actor from the selection by name, GUID or numeric ID.", ScriptMode::Any,
		    DecodeSelectAdd, ExecuteSelectAdd});
		registry.Add({"select-all", Schema(""), "Select every live actor.", ScriptMode::Any, DecodeSelectAll, ExecuteSelectAll});
		registry.Add({"copy", Schema(""), "Copy the selection.", ScriptMode::Any, DecodeSelectAll, ExecuteCopy});
		registry.Add({"expect-selection-count", Schema("count:uint"), "Assert the number of selected actors.", ScriptMode::Any, DecodeExpectSelectionCount,
		    ExecuteExpectSelectionCount});
		registry.Add({"select-rect", Schema("x1:real y1:real x2:real y2:real"), "Select actors within a world-coordinate rectangle.", ScriptMode::Any,
		    DecodeSelectRect, ExecuteSelectRect});
		registry.Add({"move-selection", Schema("x:real y:real"), "Translate the selection by the supplied offset.", ScriptMode::Any, DecodeMoveSelection,
		    ExecuteMoveSelection});
		registry.Add(
		    {"paste", Schema("[x:real] [y:real]", {0, 2}), "Paste at the supplied position or the world view center; supply both coordinates or neither.",
		        ScriptMode::Any, DecodeMoveSelection, ExecutePaste});
		registry.Add({"select-type", Schema("type:string [index:uint]"), "Select the zero-based occurrence of an actor type (defaults to zero).",
		    ScriptMode::Any, DecodeSelectType, ExecuteSelectType});
	}
} // namespace vega
