#include "editor/ScriptExecution.h"
#include "editor/JsonDocument.h"
#include "editor/ContentPath.h"
#include <cctype>

namespace vega
{
	namespace
	{
		EditorScriptParseResult DecodeContentValidate(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 1) return bad();
			return {command, {}};
		}
		EditorScriptParseResult DecodeContentOpen(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			const auto& rawTokens = arguments.rawTokens;
			const auto& verb = arguments.tokens[0];
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };

			const std::size_t arity = verb == "expect-content" ? 5 : verb == "content-new" || verb == "content-rename" ? 4 : 3;
			if (tokens.size() != arity) return bad();
			for (std::size_t i = 1; i < tokens.size(); ++i)
				if (tokens[i].empty() && !(verb == "content-set" && i == 2) && !(verb == "expect-content" && i == 4)) return bad();
			if (verb != "content-set")
				for (unsigned char c : tokens[1])
					if (!std::isalnum(c) && c != '_' && c != '-') return bad();
			if (verb == "content-set" || verb == "expect-content")
			{
				const std::size_t index = verb == "content-set" ? 1 : 3;
				try
				{
					ContentPointer(tokens[index]);
					const auto value = nlohmann::ordered_json::parse(rawTokens[index + 1]);
					if (value.is_null()) return bad();
				}
				catch (const std::exception&)
				{
					return bad();
				}
				command.text = tokens[1];
				if (verb == "expect-content")
				{
					command.property = tokens[2];
					command.values = {tokens[3], rawTokens[4]};
				}
				else command.values = {rawTokens[2]};
			}
			else
			{
				command.text = tokens[1];
				command.property = tokens[2];
				if (tokens.size() == 4) command.values = {tokens[3]};
			}
			return {command, {}};
		}
		void ExecuteContentValidate(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			auto& detail = execution.detail;
			success = context.contentCommand && context.contentCommand(command, detail);
		}
	} // namespace
	void RegisterContentScriptCommands(ScriptCommandRegistry& registry)
	{
		registry.Add({"content-validate", Schema(""), "Validate content definitions and their references.", ScriptMode::Any, DecodeContentValidate,
		    ExecuteContentValidate});
		registry.Add({"content-save", Schema(""), "Save edited content catalogs.", ScriptMode::Any, DecodeContentValidate, ExecuteContentValidate});
		registry.Add({"content-reload", Schema(""), "Reload content catalogs from the active asset root.", ScriptMode::Any, DecodeContentValidate,
		    ExecuteContentValidate});
		registry.Add({"content-open", Schema("type:text id:text"), "Open a content definition by type and ID.", ScriptMode::Any, DecodeContentOpen,
		    ExecuteContentValidate});
		registry.Add({"content-new", Schema("type:text template:text id:text"), "Create a content definition from a named template.", ScriptMode::Any,
		    DecodeContentOpen, ExecuteContentValidate});
		registry.Add({"content-rename", Schema("type:text id:text new-id:text"), "Rename a content definition and update references.", ScriptMode::Any,
		    DecodeContentOpen, ExecuteContentValidate});
		registry.Add({"content-set", Schema("path:text value:json"), "Set a content field to a JSON value.", ScriptMode::Any, DecodeContentOpen,
		    ExecuteContentValidate});
		registry.Add({"expect-content", Schema("type:text id:text path:text value:json"), "Assert a field in a content definition.", ScriptMode::Any,
		    DecodeContentOpen, ExecuteContentValidate});
	}
} // namespace vega
