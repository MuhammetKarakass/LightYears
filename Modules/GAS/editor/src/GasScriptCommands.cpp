#include <gas/GasScriptCommands.h>
#include "editor/EditorScript.h"
#include "editor/ContentPath.h"
#include "editor/SkyHandle.h"
#include <cctype>
#include <nlohmann/json.hpp>

namespace vega
{
	namespace
	{
		EditorScriptParseResult DecodeGasFilter(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 3 ||
				(tokens[1] != "abilities" && tokens[1] != "effects" && tokens[1] != "attributes" && tokens[1] != "tags" && tokens[1] != "events"))
				return bad();
			command.text = tokens[1];
			command.property = tokens[2];
			return {command, {}};
		}
		EditorScriptParseResult DecodeGasPanel(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			const auto& verb = arguments.tokens[0];
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 2 || tokens[1].empty()) return bad();
			if (verb == "gas-panel" && tokens[1] != "on" && tokens[1] != "off") return bad();
			command.text = tokens[1];
			return {command, {}};
		}
		EditorScriptParseResult DecodeGasSelectPlayer(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 1) return bad();
			return {command, {}};
		}
		EditorScriptParseResult DecodeExpectAbility(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			const auto& verb = arguments.tokens[0];
			const auto real = [&arguments](std::size_t i, float& v) { return arguments.Real(i, v); };
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != (verb == "expect-attribute" ? 4u : 3u) || tokens[1].empty()) return bad();
			command.text = tokens[1];
			command.property = tokens[2];
			if (verb == "expect-attribute")
			{
				if ((tokens[2] != "==" && tokens[2] != "<" && tokens[2] != ">" && tokens[2] != "<=" && tokens[2] != ">=") || !real(3, command.factor))
					return bad();
			}
			else if (verb == "expect-ability")
			{
				if (tokens[2] != "ready" && tokens[2] != "cooldown" && tokens[2] != "active") return bad();
			}
			else
			{
				if (tokens[2] != "true" && tokens[2] != "false") return bad();
			}
			return {command, {}};
		}
	} // namespace
	void RegisterGasScriptCommands(ScriptCommandRegistry& registry, ScriptHandler handler)
	{
		registry.Add({"gas-filter", Schema("section:abilities|effects|attributes|tags|events filter:string"),
					  "Set a debugger section filter; an empty string clears it.", ScriptMode::Any, DecodeGasFilter, handler});
		registry.Add({"gas-panel", Schema("visible:on|off"), "Show or hide the ability system debugger.", ScriptMode::Any, DecodeGasPanel, handler});
		registry.Add(
			{"gas-select-player", Schema(""), "Select the player through the installed GAS resolver.", ScriptMode::Play, DecodeGasSelectPlayer, handler});
		registry.Add({"gas-reset-cooldowns", Schema(""), "Clear ability cooldowns on the debugger target.", ScriptMode::Play, DecodeGasSelectPlayer, handler});
		registry.Add({"gas-apply-effect", Schema("id:text"), "Apply a registered effect to the debugger target.", ScriptMode::Play, DecodeGasPanel, handler});
		registry.Add({"gas-remove-effect", Schema("id:text"), "Remove matching effects from the debugger target.", ScriptMode::Play, DecodeGasPanel, handler});
		registry.Add({"gas-activate", Schema("id:text"), "Activate a non-passive ability on the debugger target.", ScriptMode::Play, DecodeGasPanel, handler});
		registry.Add({"expect-ability", Schema("id:text state:ready|cooldown|active"), "Assert an ability state on the debugger target.", ScriptMode::Play,
					  DecodeExpectAbility, handler});
		registry.Add({"expect-effect", Schema("id:text exists:true|false"), "Assert whether the debugger target has an effect.", ScriptMode::Play,
					  DecodeExpectAbility, handler});
		registry.Add({"expect-tag", Schema("id:text exists:true|false"), "Assert whether the debugger target owns a tag.", ScriptMode::Play,
					  DecodeExpectAbility, handler});
		registry.Add({"expect-attribute", Schema("id:text comparison:==|<|>|<=|>= value:real"), "Compare a debugger attribute value with a finite number.",
					  ScriptMode::Play, DecodeExpectAbility, handler});
	}
} // namespace vega
