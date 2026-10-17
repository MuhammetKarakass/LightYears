#include "editor/ScriptExecution.h"
#include <framework/scene/SceneSettings.h>

namespace vega
{
	namespace
	{
		EditorScriptParseResult DecodeSetSetting(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			const auto& rawTokens = arguments.rawTokens;
			const auto& verb = arguments.tokens[0];
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };

			const bool reset = verb == "reset-setting";
			if (tokens.size() != (reset ? 2u : 3u) || tokens[1].empty()) return bad();
			command.text = tokens[1];
			if (!reset)
			{
				try
				{
					const auto value = nlohmann::json::parse(rawTokens[2]);
					// Optional settings use null for their disabled/default state.
					if (value.is_object()) return bad();
				}
				catch (const std::exception&)
				{
					return bad();
				}
				command.values = {rawTokens[2]};
			}
			return {command, {}};
		}
		EditorScriptParseResult DecodeExpectBackdrop(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 2 || tokens[1].empty()) return bad();
			command.text = tokens[1];
			return {command, {}};
		}
		void ExecuteSetSetting(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& history = execution.history;
			auto& success = execution.success;
			auto& detail = execution.detail;
			const auto value = command.operation == "reset-setting" ? std::optional<nlohmann::json>{}
			                                                        : std::optional<nlohmann::json>{nlohmann::json::parse(command.values[0])};
			const auto& schema = context.Engine().SceneSettings();
			const auto* field = schema.Find(command.text);
			const bool noChange = context.sceneSettings && field &&
			    (value ? field->Validate(*value).empty() && schema.Value(*context.sceneSettings, command.text) == *value
			           : !context.sceneSettings->contains(command.text));
			success =
			    !context.playMode && !context.transitioning && (noChange || history.Execute(std::make_unique<SetSceneSettingCommand>(command.text, value)));
			detail = success ? "Setting updated" : context.settingError;
		}
		void ExecuteExpectSetting(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			auto& detail = execution.detail;
			if (context.sceneSettings)
			{
				const auto actual = context.Engine().SceneSettings().Value(*context.sceneSettings, command.text);
				success = actual == nlohmann::json::parse(command.values[0]);
				detail = "Setting " + command.text + " = " + actual.dump();
			}
		}
		void ExecuteExpectBackdrop(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			auto& detail = execution.detail;
			if (context.backdropState)
			{
				const auto actual = context.backdropState();
				success = actual == command.text;
				detail = "Procedural backdrop stop = " + actual;
			}
		}
	} // namespace
	void RegisterSettingsScriptCommands(ScriptCommandRegistry& registry)
	{
		registry.Add({"set-setting", Schema("key:text value:json"), "Set a scene setting to a JSON value; null selects an optional default.", ScriptMode::Edit,
		    DecodeSetSetting, ExecuteSetSetting});
		registry.Add({"expect-setting", Schema("key:text value:json"), "Assert an effective scene setting value.", ScriptMode::Any, DecodeSetSetting,
		    ExecuteExpectSetting});
		registry.Add(
		    {"reset-setting", Schema("key:text"), "Remove an authored scene setting override.", ScriptMode::Edit, DecodeSetSetting, ExecuteSetSetting});
		registry.Add({"expect-backdrop", Schema("id:text"), "Assert the game-provided scene backdrop state.", ScriptMode::Any, DecodeExpectBackdrop,
		    ExecuteExpectBackdrop});
	}
} // namespace vega
