#include <editor/SkyScriptCommands.h>
#include "editor/EditorScript.h"
#include "editor/ContentPath.h"
#include "editor/SkyHandle.h"
#include <cctype>
#include <nlohmann/json.hpp>

namespace vega
{
	namespace
	{
		EditorScriptParseResult DecodeSkyOpen(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			const auto& verb = arguments.tokens[0];
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != (verb == "sky-save" ? 1u : 2u) || (tokens.size() == 2 && tokens[1].empty())) return bad();
			if (tokens.size() == 2) command.text = tokens[1];
			return {command, {}};
		}
		EditorScriptParseResult DecodeSkySet(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			const auto& rawTokens = arguments.rawTokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 3) return bad();
			try
			{
				ContentPointer(tokens[1]);
				const auto value = nlohmann::json::parse(rawTokens[2]);
				if (value.is_null() || value.is_object()) return bad();
			}
			catch (const std::exception&)
			{
				return bad();
			}
			command.text = tokens[1];
			command.values = {rawTokens[2]};
			return {command, {}};
		}
		EditorScriptParseResult DecodeSkyDrag(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			const auto real = [&arguments](std::size_t i, float& v) { return arguments.Real(i, v); };
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 4 || !IsSkyHandle(tokens[1]) || !real(2, command.location.x) || !real(3, command.location.y)) return bad();
			command.text = tokens[1];
			return {command, {}};
		}
	} // namespace
	void RegisterSkyScriptCommands(ScriptCommandRegistry& registry, ScriptHandler handler)
	{
		registry.Add({"sky-open", Schema("id:text"), "Open a sky by its opaque asset ID.", ScriptMode::Edit, DecodeSkyOpen, handler});
		registry.Add({"capture-sky", Schema("path:text"), "Capture the sky preview to an image file.", ScriptMode::Edit, DecodeSkyOpen, handler});
		registry.Add({"sky-save", Schema(""), "Save the authored sky document.", ScriptMode::Edit, DecodeSkyOpen, handler});
		registry.Add({"sky-set", Schema("path:string value:json"), "Set a sky field using a JSON scalar or array.", ScriptMode::Edit, DecodeSkySet, handler});
		registry.Add({"expect-sky", Schema("path:string value:json"), "Assert a sky field value.", ScriptMode::Edit, DecodeSkySet, handler});
		registry.Add({"sky-drag", Schema("handle:text x:real y:real"), "Move a sky handle by a pixel delta.", ScriptMode::Edit, DecodeSkyDrag, handler});
	}
} // namespace vega
