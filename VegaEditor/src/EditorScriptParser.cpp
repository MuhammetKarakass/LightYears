#include "editor/EditorScript.h"
#include <cctype>

namespace vega
{
	EditorScriptParseResult ParseEditorScriptLine(const std::string& line, const ScriptCommandRegistry& registry)
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
			else if (line[cursor] == '[' || line[cursor] == '{')
			{
				// Composite JSON is one argument, including spaces and quoted keys.
				// JSON parsing below checks matching delimiters and scalar validity.
				int depth = 0;
				bool quoted = false, escaped = false;
				do
				{
					const char c = line[cursor++];
					token += c;
					if (quoted)
					{
						if (escaped) escaped = false;
						else if (c == '\\') escaped = true;
						else if (c == '"') quoted = false;
					}
					else if (c == '"') quoted = true;
					else if (c == '[' || c == '{') ++depth;
					else if (c == ']' || c == '}') --depth;
				} while (cursor < line.size() && (depth > 0 || quoted));
				if (depth != 0 || quoted || (cursor < line.size() && !std::isspace(static_cast<unsigned char>(line[cursor])) && line[cursor] != '#')) return { {}, "Invalid JSON argument" };
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
		const auto* definition = registry.Find(tokens[0]);
		if (!definition) return {{}, "Unknown command: " + tokens[0]};
		const ScriptArguments arguments{std::move(tokens), std::move(rawTokens)};
		if (!definition->schema.Validate(arguments)) return {{}, "Invalid arguments"};
		return definition->decode(arguments);
	}
}
