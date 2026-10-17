#include "editor/ScriptExecution.h"
#include <algorithm>
#include <framework/World.h>
#include <framework/scene/SceneEntityComponent.h>
#include <cmath>

namespace vega
{
	namespace
	{
		EditorScriptParseResult DecodeCreateMany(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			const auto integer = [&arguments](std::size_t i, unsigned int& v) { return arguments.Integer(i, v); };
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 3 || tokens[1].empty() || !integer(2, command.number) || command.number > 10000) return bad();
			command.text = tokens[1];
			return {command, {}};
		}
		EditorScriptParseResult DecodeRename(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() < 2) return bad();
			for (std::size_t index = 1; index < tokens.size(); ++index)
			{
				if (index > 1) command.text += ' ';
				command.text += tokens[index];
			}
			if (command.text.empty()) return bad();
			return {command, {}};
		}
		EditorScriptParseResult DecodeOpenScene(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			const auto& verb = arguments.tokens[0];
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };

			if (verb == "save-scene" && tokens.size() == 1) { return {command, {}}; }
			if (tokens.size() != 2 || tokens[1].empty()) return bad();
			command.text = tokens[1];
			if (verb == "expect-mode" && command.text != "edit" && command.text != "play") return bad();
			if (verb == "expect-scene-dirty" && command.text != "true" && command.text != "false") return bad();
			return {command, {}};
		}
		EditorScriptParseResult DecodeExpectEntityCount(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			const auto integer = [&arguments](std::size_t i, unsigned int& v) { return arguments.Integer(i, v); };
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 2 || !integer(1, command.number)) return bad();
			return {command, {}};
		}
		EditorScriptParseResult DecodeNewScene(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 1) return bad();
			return {command, {}};
		}
		void ExecuteCreateMany(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& history = execution.history;
			auto& success = execution.success;
			const unsigned int columns = std::max(1u, static_cast<unsigned int>(std::ceil(std::sqrt(command.number))));
			const auto centre = context.world->GetWorldView().getCenter();
			success = true;
			for (unsigned int index = 0; index < command.number; ++index)
			{
				if (!CreateSceneEntity(context, history, command.text))
				{
					success = false;
					break;
				}
				const auto created = context.selection.Get();
				const sf::Vector2f location = centre +
				    sf::Vector2f{(static_cast<float>(index % columns) - (columns - 1) * 0.5f) * 48.f,
				        (static_cast<float>(index / columns) - (columns - 1) * 0.5f) * 48.f};
				if (!created || !history.Execute(std::make_unique<SetPropertyCommand>(created->GetUniqueID(), std::nullopt, "Location", location)))
				{
					success = false;
					break;
				}
			}
		}
		void ExecuteRename(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& history = execution.history;
			auto& success = execution.success;
			success = RenameSceneEntity(context, history, command.text);
		}
		void ExecuteSelectName(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			auto& detail = execution.detail;
			unsigned int count = 0;
			for (const auto& weak : context.world->GetActorsByTypeIncludingPending<vega::Actor>())
			{
				const auto candidate = weak.lock();
				const auto* identity = candidate ? candidate->FindComponent<vega::SceneEntityComponent>() : nullptr;
				if (!identity || candidate->GetIsPendingDestroy()) continue;
				++count;
				if (command.operation == "select-name" && identity->name == command.text)
				{
					context.selection.Select(candidate);
					success = true;
					break;
				}
			}
			if (command.operation == "expect-entity-count")
			{
				success = count == command.number;
				detail = "count " + std::to_string(count) + " expected " + std::to_string(command.number);
			}
		}
		void ExecuteOpenScene(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			auto& detail = execution.detail;
			const std::string action = command.operation == "new-scene" ? "new"
			    : command.operation == "open-scene"                     ? "open"
			    : command.operation == "save-scene"                     ? "save"
			    : command.operation == "play-mode"                      ? "play"
			                                                            : "stop";
			success = context.sceneAction && context.sceneAction(action, command.text);
			detail = success ? "Scene " + action : context.sceneError;
		}
		void ExecuteCreate(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& history = execution.history;
			auto& success = execution.success;
			success = CreateSceneEntity(context, history, command.text);
		}
		void ExecuteExpectMode(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			success = context.playMode == (command.text == "play");
		}
		void ExecuteExpectSceneDirty(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			success = context.sceneDirty && context.sceneDirty() == (command.text == "true");
		}
		void ExecuteDelete(ScriptExecution& execution, const EditorScriptCommand&)
		{
			auto& context = execution.context;
			auto& history = execution.history;
			auto& success = execution.success;
			success = DeleteSelection(context, history);
		}
		void ExecuteDuplicate(ScriptExecution& execution, const EditorScriptCommand&)
		{
			auto& context = execution.context;
			auto& history = execution.history;
			auto& success = execution.success;
			success = DuplicateSceneEntity(context, history);
		}
	} // namespace
	void RegisterSceneScriptCommands(ScriptCommandRegistry& registry)
	{
		registry.Add({"create-many", Schema("type:text count:uint"), "Create up to 10000 entities in a grid around the world view center.", ScriptMode::Any,
		    DecodeCreateMany, ExecuteCreateMany});
		registry.Add({"rename", Schema("name:string..."), "Rename the selected entity; unquoted words are joined with spaces.", ScriptMode::Any, DecodeRename,
		    ExecuteRename});
		registry.Add({"select-name", Schema("name:string..."), "Select an entity by name; unquoted words are joined with spaces.", ScriptMode::Any,
		    DecodeRename, ExecuteSelectName});
		registry.Add(
		    {"open-scene", Schema("path:text"), "Open a scene after checking for unsaved changes.", ScriptMode::Edit, DecodeOpenScene, ExecuteOpenScene});
		registry.Add(
		    {"save-scene", Schema("[path:text]"), "Save the active scene, optionally to a new path.", ScriptMode::Edit, DecodeOpenScene, ExecuteOpenScene});
		registry.Add({"create", Schema("type:text"), "Create a reflected entity by type.", ScriptMode::Edit, DecodeOpenScene, ExecuteCreate});
		registry.Add({"expect-mode", Schema("mode:edit|play"), "Assert whether the editor is in Edit or Play mode.", ScriptMode::Any, DecodeOpenScene,
		    ExecuteExpectMode});
		registry.Add({"expect-scene-dirty", Schema("dirty:true|false"), "Assert whether the scene has unsaved changes.", ScriptMode::Any, DecodeOpenScene,
		    ExecuteExpectSceneDirty});
		registry.Add({"expect-entity-count", Schema("count:uint"), "Assert the number of authored entities.", ScriptMode::Any, DecodeExpectEntityCount,
		    ExecuteSelectName});
		registry.Add({"new-scene", Schema(""), "Create a new scene after checking for unsaved changes.", ScriptMode::Edit, DecodeNewScene, ExecuteOpenScene});
		registry.Add({"delete", Schema(""), "Delete the selected entities.", ScriptMode::Any, DecodeNewScene, ExecuteDelete});
		registry.Add({"duplicate", Schema(""), "Duplicate the selected entity.", ScriptMode::Any, DecodeNewScene, ExecuteDuplicate});
		registry.Add({"play-mode", Schema(""), "Start Play using a copy of the edit scene.", ScriptMode::Any, DecodeNewScene, ExecuteOpenScene});
		registry.Add({"stop", Schema(""), "Stop Play and restore the edit scene.", ScriptMode::Any, DecodeNewScene, ExecuteOpenScene});
	}
} // namespace vega
