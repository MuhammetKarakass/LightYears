#include "editor/ScriptCommandRegistry.h"
#include "editor/EditorScript.h"
#include "editor/ScriptExecution.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <nlohmann/json.hpp>

namespace vega
{
	ScriptCommandDefinition TokenScriptCommand(std::string name, ScriptSchema schema, std::string help, ScriptMode mode, TokenScriptHandler handler)
	{
		return {std::move(name),
				std::move(schema),
				std::move(help),
				mode,
				[](const ScriptArguments& arguments) -> EditorScriptParseResult
				{
					EditorScriptCommand command;
					command.operation = arguments.tokens[0];
					command.values.assign(arguments.tokens.begin() + 1, arguments.tokens.end());
					return {std::move(command), {}};
				},
				[handler = std::move(handler)](ScriptExecution& execution, const EditorScriptCommand& command)
				{ execution.success = handler(execution.context, execution.history, command.values, execution.detail); }};
	}
	bool ScriptArguments::Integer(std::size_t index, unsigned int& value) const
	{
		const auto& token = tokens.at(index);
		const auto result = std::from_chars(token.data(), token.data() + token.size(), value);
		return result.ec == std::errc{} && result.ptr == token.data() + token.size();
	}
	bool ScriptArguments::Real(std::size_t index, float& value) const
	{
		std::istringstream input{tokens.at(index)};
		input.imbue(std::locale::classic());
		return (input >> value) && input.eof() && std::isfinite(value);
	}
	ScriptSchema Schema(const std::string& declaration, std::vector<std::size_t> counts)
	{
		ScriptSchema schema;
		schema.counts = std::move(counts);
		std::istringstream input{declaration};
		for (std::string word; input >> word;)
		{
			ScriptArgument argument;
			argument.optional = word.front() == '[';
			if (argument.optional) word = word.substr(1, word.size() - 2);
			argument.repeated = word.size() >= 3 && word.substr(word.size() - 3) == "...";
			if (argument.repeated) word.resize(word.size() - 3);
			const auto colon = word.find(':');
			if (colon == std::string::npos) throw std::invalid_argument("Script argument needs name:type");
			argument.name = word.substr(0, colon);
			argument.type = word.substr(colon + 1);
			schema.arguments.push_back(std::move(argument));
		}
		return schema;
	}
	bool ScriptSchema::Validate(const ScriptArguments& input) const
	{
		if (input.tokens.empty()) return false;
		const auto count = input.tokens.size() - 1;
		if (!counts.empty() && std::find(counts.begin(), counts.end(), count) == counts.end()) return false;
		const auto required = std::count_if(arguments.begin(), arguments.end(), [](const auto& a) { return !a.optional; });
		if (count < static_cast<std::size_t>(required)) return false;
		if (count > arguments.size() && (arguments.empty() || !arguments.back().repeated)) return false;
		for (std::size_t i = 0; i < count; ++i)
		{
			const auto& type = arguments[std::min(i, arguments.size() - 1)].type;
			const auto& token = input.tokens[i + 1];
			unsigned int integer = 0;
			float real = 0.f;
			if (type == "text" && token.empty()) return false;
			if (type == "uint" && !input.Integer(i + 1, integer)) return false;
			if (type == "real" && !input.Real(i + 1, real)) return false;
			if (type == "json")
			{
				try
				{
					const auto value = nlohmann::json::parse(input.rawTokens[i + 1]);
					(void)value;
				}
				catch (const std::exception&)
				{
					return false;
				}
			}
			if (type != "text" && type != "string" && type != "uint" && type != "real" && type != "json" && type != "value")
			{
				std::istringstream choices{type};
				bool matched = false;
				for (std::string choice; std::getline(choices, choice, '|');) matched |= choice == token;
				if (!matched) return false;
			}
		}
		return true;
	}
	std::string ScriptSchema::Usage() const
	{
		std::string text;
		for (const auto& argument : arguments)
		{
			text += ' ';
			if (argument.optional) text += '[';
			text += argument.name + ':' + argument.type;
			if (argument.repeated) text += "...";
			if (argument.optional) text += ']';
		}
		return text;
	}
	void ScriptCommandRegistry::Add(ScriptCommandDefinition definition)
	{
		if (definition.name.empty() || definition.help.empty() || !definition.decode || !definition.execute)
			throw std::invalid_argument("Incomplete script command declaration");
		const auto name = definition.name;
		if (!mCommands.emplace(name, std::move(definition)).second) throw std::invalid_argument("Duplicate script command: " + name);
	}
	const ScriptCommandDefinition* ScriptCommandRegistry::Find(const std::string& name) const
	{
		const auto found = mCommands.find(name);
		return found == mCommands.end() ? nullptr : &found->second;
	}
	std::string ScriptCommandRegistry::Help(const std::string& name) const
	{
		if (!name.empty())
		{
			const auto* command = Find(name);
			return command ? command->name + command->schema.Usage() + " -- " + command->help : "Unknown command: " + name;
		}
		std::string text;
		for (const auto& entry : mCommands)
		{
			if (!text.empty()) text += ", ";
			text += entry.first;
		}
		return text;
	}
	std::string ScriptCommandRegistry::Markdown() const
	{
		std::string text = "# Editor script commands\n\nGenerated from the command registry. Arguments accept quoted strings; quotes preserve spaces, hashes "
						   "and empty strings. JSON arguments preserve their original spelling. `#` starts a comment outside quotes. Optional arguments are in "
						   "brackets; `...` repeats an argument. `text` is nonempty; `string` and `value` may be empty. Numeric arguments are finite; `uint` "
						   "is an unsigned integer. Mode requirements are checked by the handler to preserve command-specific diagnostics.\n\n";
		for (const auto& entry : mCommands)
		{
			const auto& command = entry.second;
			text += "## `" + command.name + command.schema.Usage() + "`\n\n" + command.help + " Mode: " +
					(command.mode == ScriptMode::Edit	? "Edit"
					 : command.mode == ScriptMode::Play ? "Play"
														: "Any") +
					".\n\n";
		}
		return text;
	}
} // namespace vega
