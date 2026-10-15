#include "editor/EditorScript.h"
#include <charconv>
#include <cmath>
#include <locale>
#include <sstream>

namespace vega
{
	EditorScriptParseResult ParseEditorScriptLine(const std::string& line)
	{
		std::istringstream stream{ line.substr(0, line.find('#')) };
		stream.imbue(std::locale::classic());
		std::vector<std::string> tokens;
		for (std::string token; stream >> token;) tokens.push_back(token);
		if (tokens.empty()) return {};
		EditorScriptCommand command;
		const auto bad = []() { return EditorScriptParseResult{ {}, "Invalid arguments" }; };
		const auto integer = [&tokens](std::size_t index, unsigned int& value)
		{
			const auto& token = tokens[index];
			const auto result = std::from_chars(token.data(), token.data() + token.size(), value);
			return result.ec == std::errc{} && result.ptr == token.data() + token.size();
		};
		const auto real = [&tokens](std::size_t index, float& value)
		{
			std::istringstream input{ tokens[index] };
			input.imbue(std::locale::classic());
			return (input >> value) && input.eof() && std::isfinite(value);
		};
		const std::string& verb = tokens[0];
		if (verb == "pause") command.operation = ScriptOperation::Pause;
		else if (verb == "play") command.operation = ScriptOperation::Play;
		else if (verb == "undo") command.operation = ScriptOperation::Undo;
		else if (verb == "redo") command.operation = ScriptOperation::Redo;
		else if (verb == "quit") command.operation = ScriptOperation::Quit;
		else if (verb == "remember-location") command.operation = ScriptOperation::RememberLocation;
		else if (verb == "expect-remembered-location") command.operation = ScriptOperation::ExpectRememberedLocation;
		else if (verb == "wait" || verb == "select")
		{
			if (tokens.size() != 2 || !integer(1, command.number)) return bad();
			command.operation = verb == "wait" ? ScriptOperation::Wait : ScriptOperation::Select;
			return { command, {} };
		}
		else if (verb == "select-type")
		{
			if (tokens.size() < 2 || tokens.size() > 3 || (tokens.size() == 3 && !integer(2, command.number))) return bad();
			command.operation = ScriptOperation::SelectType;
			command.text = tokens[1];
			return { command, {} };
		}
		else if (verb == "move" || verb == "expect-location")
		{
			const bool expect = verb == "expect-location";
			if (tokens.size() < 3 || tokens.size() > (expect ? 4u : 3u) ||
				!real(1, command.location.x) || !real(2, command.location.y) ||
				(tokens.size() == 4 && (!real(3, command.tolerance) || command.tolerance < 0.f))) return bad();
			command.operation = expect ? ScriptOperation::ExpectLocation : ScriptOperation::Move;
			return { command, {} };
		}
		else if (verb == "rotate")
		{
			if (tokens.size() != 2 || !real(1, command.degrees)) return bad();
			command.operation = ScriptOperation::Rotate;
			return { command, {} };
		}
		else if (verb == "expect-selected" || verb == "capture" || verb == "expect-count")
		{
			if (tokens.size() != (verb == "expect-count" ? 3u : 2u)) return bad();
			command.text = tokens[1];
			if (verb == "expect-count")
			{
				if (!integer(2, command.number)) return bad();
				command.operation = ScriptOperation::ExpectCount;
			}
			else command.operation = verb == "capture" ? ScriptOperation::Capture : ScriptOperation::ExpectSelected;
			return { command, {} };
		}
		else return { {}, "Unknown command: " + verb };
		if (tokens.size() != 1) return bad();
		return { command, {} };
	}
}
