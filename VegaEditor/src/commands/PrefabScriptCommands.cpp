#include "editor/ScriptExecution.h"
#include "editor/EditorPrefabs.h"
#include <framework/World.h>
#include <framework/scene/SceneEntityComponent.h>

namespace vega
{
	namespace
	{
		EditorScriptParseResult DecodeOpenPrefab(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			const auto& verb = arguments.tokens[0];
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != (verb == "open-prefab" ? 2u : 1u)) return bad();
			if (verb == "open-prefab")
			{
				if (tokens[1].empty()) return bad();
				command.text = tokens[1];
			}
			return {command, {}};
		}
		EditorScriptParseResult DecodeCreatePrefab(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			const auto& verb = arguments.tokens[0];
			const auto real = [&arguments](std::size_t i, float& v) { return arguments.Real(i, v); };
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };

			if (verb == "reload-prefabs")
			{
				if (tokens.size() != 1) return bad();
			}
			else
			{
				if (tokens.size() < 2 || tokens[1].empty()) return bad();
				command.text = tokens[1];
				if (verb == "place-prefab")
				{
					if (tokens.size() != 2 && tokens.size() != 4) return bad();
					if (tokens.size() == 4 && (!real(2, command.location.x) || !real(3, command.location.y))) return bad();
					command.number = tokens.size() == 4 ? 1 : 0;
				}
				else if (verb == "create-prefab" || verb == "expect-prefab")
				{
					if (tokens.size() != 2) return bad();
				}
				else
				{
					if (tokens.size() != (verb == "expect-overridden" ? 4u : 3u) || tokens[2].empty()) return bad();
					command.property = tokens[2];
					if (verb == "expect-overridden")
					{
						if (tokens[3] != "true" && tokens[3] != "false") return bad();
						command.overwrite = tokens[3] == "true";
					}
					else
						command.operation = verb == "create-variant" ? "create-variant"
						    : verb == "apply-prefab-prop"            ? "apply-prefab-prop"
						                                             : "revert-prefab-prop";
				}
			}
			return {command, {}};
		}
		void ExecuteOpenPrefab(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			success = context.prefabAction &&
			    context.prefabAction(command.operation == "open-prefab" ? "open"
			            : command.operation == "save-prefab"            ? "save"
			                                                            : "close",
			        command.text);
		}
		void ExecuteCreatePrefab(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& history = execution.history;
			auto& success = execution.success;
			success = CreatePrefab(context, history, command.text);
		}
		void ExecuteExpectPrefab(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& success = execution.success;
			auto actor = execution.context.selection.Get();
			if (actor && actor->GetWorld() != execution.context.world) actor.reset();
			if (const auto* identity = actor ? actor->FindComponent<vega::SceneEntityComponent>() : nullptr)
				success = identity->prefab == (command.text == "none" ? "" : command.text);
		}
		void ExecuteReloadPrefabs(ScriptExecution& execution, const EditorScriptCommand&)
		{
			auto& context = execution.context;
			auto& history = execution.history;
			auto& success = execution.success;
			success = RefreshPrefabs(context, history);
		}
		void ExecutePlacePrefab(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& history = execution.history;
			auto& success = execution.success;
			success = PlacePrefab(context, history, command.text, command.number ? command.location : context.world->GetWorldView().getCenter());
		}
		void ExecuteCreateVariant(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			success = CreateVariant(context, command.text, command.property);
		}
		void ExecuteApplyPrefabProp(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& history = execution.history;
			auto& success = execution.success;
			auto actor = execution.context.selection.Get();
			if (actor && actor->GetWorld() != execution.context.world) actor.reset();
			if (actor)
				success = ChangePrefabProperty(
				    context, history, FindPropertyTarget(*actor, command.text), command.property, command.operation == "apply-prefab-prop");
		}
		void ExecuteExpectOverridden(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			auto actor = execution.context.selection.Get();
			if (actor && actor->GetWorld() != execution.context.world) actor.reset();
			if (actor) success = PrefabPropertyOverridden(context, *actor, FindPropertyTarget(*actor, command.text), command.property) == command.overwrite;
		}
	} // namespace
	void RegisterPrefabScriptCommands(ScriptCommandRegistry& registry)
	{
		registry.Add({"open-prefab", Schema("path:text"), "Open a prefab in the prefab editor.", ScriptMode::Edit, DecodeOpenPrefab, ExecuteOpenPrefab});
		registry.Add({"create-prefab", Schema("path:text"), "Save the selected entity as a prefab.", ScriptMode::Any, DecodeCreatePrefab, ExecuteCreatePrefab});
		registry.Add(
		    {"expect-prefab", Schema("path:text"), "Assert the selected entity prefab path.", ScriptMode::Any, DecodeCreatePrefab, ExecuteExpectPrefab});
		registry.Add({"save-prefab", Schema(""), "Save the active prefab.", ScriptMode::Edit, DecodeOpenPrefab, ExecuteOpenPrefab});
		registry.Add({"close-prefab", Schema(""), "Close the active prefab.", ScriptMode::Edit, DecodeOpenPrefab, ExecuteOpenPrefab});
		registry.Add(
		    {"reload-prefabs", Schema(""), "Reload prefab definitions and refresh instances.", ScriptMode::Any, DecodeCreatePrefab, ExecuteReloadPrefabs});
		registry.Add({"place-prefab", Schema("path:text [x:real] [y:real]", {1, 3}),
		    "Place a prefab at the supplied position or world view center; supply both coordinates or neither.", ScriptMode::Any, DecodeCreatePrefab,
		    ExecutePlacePrefab});
		registry.Add({"create-variant", Schema("parent:text path:text"), "Create a prefab variant from its parent.", ScriptMode::Any, DecodeCreatePrefab,
		    ExecuteCreateVariant});
		registry.Add({"apply-prefab-prop", Schema("target:text property:text"), "Apply an instance property to its prefab.", ScriptMode::Any,
		    DecodeCreatePrefab, ExecuteApplyPrefabProp});
		registry.Add({"revert-prefab-prop", Schema("target:text property:text"), "Revert an instance property to its prefab value.", ScriptMode::Any,
		    DecodeCreatePrefab, ExecuteApplyPrefabProp});
		registry.Add({"expect-overridden", Schema("target:text property:text overridden:true|false"),
		    "Assert whether an instance property overrides its prefab.", ScriptMode::Any, DecodeCreatePrefab, ExecuteExpectOverridden});
	}
} // namespace vega
