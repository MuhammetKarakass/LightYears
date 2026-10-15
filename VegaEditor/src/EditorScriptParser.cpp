#include "editor/EditorScript.h"
#include <charconv>
#include <cmath>
#include <locale>
#include <sstream>
#include <cctype>

namespace vega
{
	EditorScriptParseResult ParseEditorScriptLine(const std::string& line)
	{
		std::vector<std::string> tokens;
		// Comments only start outside quotes. Quoted arguments preserve spaces,
		// hashes and empty strings; backslashes are literal except before a quote or backslash.
		std::size_t cursor = 0;
		while (cursor < line.size())
		{
			if (std::isspace(static_cast<unsigned char>(line[cursor]))) { ++cursor; continue; }
			if (line[cursor] == '#') break;
			std::string token;
			if (line[cursor] == '"')
			{
				++cursor;
				bool closed = false;
				while (cursor < line.size())
				{
					char character = line[cursor++];
					if (character == '"') { closed = true; break; }
					if (character == '\\' && cursor < line.size() && (line[cursor] == '"' || line[cursor] == '\\')) character = line[cursor++];
					token += character;
				}
				if (!closed || (cursor < line.size() && !std::isspace(static_cast<unsigned char>(line[cursor])) && line[cursor] != '#'))
					return { {}, "Invalid quoted argument" };
			}
			else
			{
				while (cursor < line.size() && !std::isspace(static_cast<unsigned char>(line[cursor])) && line[cursor] != '#')
				{
					if (line[cursor] == '"') return { {}, "Quote must start an argument" };
					token += line[cursor++];
				}
			}
			tokens.push_back(std::move(token));
		}
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
		if (verb == "set-prop" || verb == "expect-prop")
		{
			if (tokens.size() < 4 || tokens[1].empty() || tokens[2].empty()) return bad();
			command.operation = verb == "set-prop" ? ScriptOperation::SetProp : ScriptOperation::ExpectProp;
			command.text = tokens[1];
			command.property = tokens[2];
			command.values.assign(tokens.begin() + 3, tokens.end());
			// Arity and scalar types depend on the selected object's descriptor.
			return { command, {} };
		}
		else if (verb == "pause") command.operation = ScriptOperation::Pause;
		else if (verb == "play") command.operation = ScriptOperation::Play;
		else if (verb == "focus") command.operation = ScriptOperation::Focus;
		else if (verb == "expect-visible") command.operation = ScriptOperation::ExpectVisible;
		else if (verb == "camera" || verb == "expect-camera")
		{
			if (tokens.size() != 2 || (tokens[1] != "editor" && tokens[1] != "game")) return bad();
			command.text = tokens[1];
			command.operation = verb == "camera" ? ScriptOperation::Camera : ScriptOperation::ExpectCamera;
			return { command, {} };
		}
		else if (verb == "zoom")
		{
			if (tokens.size() != 2 || !real(1, command.factor) || command.factor <= 0.f) return bad();
			command.operation = ScriptOperation::Zoom;
			return { command, {} };
		}
		else if (verb == "pan")
		{
			if (tokens.size() != 3 || !real(1, command.location.x) || !real(2, command.location.y)) return bad();
			command.operation = ScriptOperation::Pan;
			return { command, {} };
		}
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
