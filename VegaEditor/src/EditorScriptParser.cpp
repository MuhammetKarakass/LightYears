#include "editor/EditorScript.h"
#include <charconv>
#include <cmath>
#include <locale>
#include <sstream>
#include <cctype>
#include <nlohmann/json.hpp>

namespace vega
{
	EditorScriptParseResult ParseEditorScriptLine(const std::string& line)
	{
		std::vector<std::string> tokens;
		std::vector<std::string> rawTokens;
		// Comments only start outside quotes. Quoted arguments preserve spaces,
		// hashes and empty strings; backslashes are literal except before a quote or backslash.
		std::size_t cursor = 0;
		while (cursor < line.size())
		{
			if (std::isspace(static_cast<unsigned char>(line[cursor]))) { ++cursor; continue; }
			if (line[cursor] == '#') break;
			const auto start = cursor;
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
			rawTokens.push_back(line.substr(start, cursor - start));
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
		if (verb == "tool" || verb == "snap" || verb == "grid" || verb == "colliders" || verb == "content-view")
		{
			if (tokens.size() != 2) return bad();
			if (verb == "tool")
			{
				if (tokens[1] != "move" && tokens[1] != "rotate") return bad();
				command.operation = ScriptOperation::Tool;
			}
			else if (verb == "content-view")
			{
				if (tokens[1] != "grid" && tokens[1] != "list") return bad();
				command.operation = ScriptOperation::ContentView;
			}
			else
			{
				if (tokens[1] != "on" && tokens[1] != "off") return bad();
				command.operation = verb == "snap" ? ScriptOperation::Snap : verb == "grid" ? ScriptOperation::Grid : ScriptOperation::Colliders;
			}
			command.text = tokens[1];
			return { command, {} };
		}
		if (verb == "add-script" || verb == "expect-script-error")
		{
			if (tokens.size() != 2 || tokens[1].empty()) return bad();
			command.operation = verb == "add-script" ? ScriptOperation::AddScript : ScriptOperation::ExpectScriptError;
			command.text = tokens[1];
			return { command, {} };
		}
		if (verb == "set-script-prop" || verb == "expect-script-prop")
		{
			if (tokens.size() < 3 || tokens[1].empty()) return bad();
			command.operation = verb == "set-script-prop" ? ScriptOperation::SetScriptProp : ScriptOperation::ExpectScriptProp;
			command.property = tokens[1];
			command.values.assign(tokens.begin() + 2, tokens.end());
			return { command, {} };
		}
		if (verb == "wait-seconds")
		{
			if (tokens.size() != 2 || !real(1, command.factor) || command.factor < 0.f || command.factor > 86400.f) return bad();
			command.operation = ScriptOperation::WaitSeconds;
			return { command, {} };
		}
		if (verb == "open-json")
		{
			if (tokens.size() != 2 || tokens[1].empty()) return bad();
			command.operation = ScriptOperation::OpenJson;
			command.text = tokens[1];
			return { command, {} };
		}
		else if (verb == "json-set" || verb == "expect-json")
		{
			if (tokens.size() != 3) return bad();
			try
			{
				const nlohmann::ordered_json::json_pointer pointer{ tokens[1] };
				const auto value = nlohmann::ordered_json::parse(rawTokens[2]);
				if (value.is_structured() || value.is_null() || (value.is_number_float() && !std::isfinite(value.get<double>()))) return bad();
			}
			catch (const std::exception&) { return bad(); }
			command.operation = verb == "json-set" ? ScriptOperation::JsonSet : ScriptOperation::ExpectJson;
			command.text = tokens[1];
			command.values = { rawTokens[2] };
			return { command, {} };
		}
		else if (verb == "expect-json-dirty")
		{
			if (tokens.size() != 2 || (tokens[1] != "true" && tokens[1] != "false")) return bad();
			command.operation = ScriptOperation::ExpectJsonDirty;
			command.text = tokens[1];
			return { command, {} };
		}
		else if (verb == "export")
		{
			if (tokens.size() < 2 || tokens.size() > 3 || tokens[1].empty() ||
				(tokens.size() == 3 && tokens[2] != "overwrite")) return bad();
			command.operation = ScriptOperation::Export;
			command.text = tokens[1];
			command.overwrite = tokens.size() == 3;
			return { command, {} };
		}
		else if (verb == "expect-export-ok") command.operation = ScriptOperation::ExpectExportOk;
		else if (verb == "json-save") command.operation = ScriptOperation::JsonSave;
		else if (verb == "json-revert") command.operation = ScriptOperation::JsonRevert;
		else if (verb == "set-prop" || verb == "expect-prop")
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
		else if (verb == "remember-rotation") command.operation = ScriptOperation::RememberRotation;
		else if (verb == "expect-rotation-changed") command.operation = ScriptOperation::ExpectRotationChanged;
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
