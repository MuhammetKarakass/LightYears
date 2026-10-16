#include "editor/ScriptExecution.h"
#include "editor/JsonCommands.h"
#include <cmath>

namespace vega
{
	namespace
	{
		EditorScriptParseResult DecodeOpenJson(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 2 || tokens[1].empty()) return bad();
			command.text = tokens[1];
			return {command, {}};
		}
		EditorScriptParseResult DecodeJsonSet(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			const auto& rawTokens = arguments.rawTokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 3) return bad();
			try
			{
				const nlohmann::ordered_json::json_pointer pointer{tokens[1]};
				const auto value = nlohmann::ordered_json::parse(rawTokens[2]);
				if (value.is_structured() || value.is_null() || (value.is_number_float() && !std::isfinite(value.get<double>()))) return bad();
			}
			catch (const std::exception&)
			{
				return bad();
			}
			command.text = tokens[1];
			command.values = {rawTokens[2]};
			return {command, {}};
		}
		EditorScriptParseResult DecodeExpectJsonDirty(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 2 || (tokens[1] != "true" && tokens[1] != "false")) return bad();
			command.text = tokens[1];
			return {command, {}};
		}
		EditorScriptParseResult DecodeJsonSave(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 1) return bad();
			return {command, {}};
		}
		EditorScriptParseResult DecodeJsonRevert(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 1) return bad();
			return {command, {}};
		}
		void ExecuteOpenJson(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			auto& detail = execution.detail;
			if (context.document && !context.document->Dirty()) success = context.document->Open(command.text);
			detail = context.document && context.document->Dirty() ? "Unsaved changes: save or revert before open-json"
			    : context.document                                 ? context.document->Error()
			                                                       : "No JSON document service";
		}
		void ExecuteJsonSet(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& history = execution.history;
			auto& success = execution.success;
			auto& detail = execution.detail;
			const auto* actual = context.document ? context.document->Get(command.text) : nullptr;
			const auto expected = Json::parse(command.values[0]);
			if (actual)
			{
				if (command.operation == "json-set")
					success = history.Execute(std::make_unique<SetJsonValueCommand>(context.document->ID(), command.text, *actual, expected));
				else success = *actual == expected && (actual->type() == expected.type() || (actual->is_number_integer() && expected.is_number_integer()));
				detail = "actual " + actual->dump() + " expected " + expected.dump();
			}
			else detail = "Unknown JSON pointer or no open document";
		}
		void ExecuteExpectJsonDirty(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			success = context.document && context.document->IsOpen() && context.document->Dirty() == (command.text == "true");
		}
		void ExecuteJsonSave(ScriptExecution& execution, const EditorScriptCommand&)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			auto& detail = execution.detail;
			success = context.document && context.document->Save();
			if (context.document) detail = context.document->Error();
		}
		void ExecuteJsonRevert(ScriptExecution& execution, const EditorScriptCommand&)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			auto& detail = execution.detail;
			success = context.document && context.document->Revert();
			if (context.document) detail = context.document->Error();
		}
	} // namespace
	void RegisterJsonScriptCommands(ScriptCommandRegistry& registry)
	{
		registry.Add({"open-json", Schema("path:text"), "Open a JSON document with no unsaved changes.", ScriptMode::Any, DecodeOpenJson, ExecuteOpenJson});
		registry.Add({"json-set", Schema("pointer:string value:json"), "Set a JSON pointer to a non-null scalar value.", ScriptMode::Any, DecodeJsonSet,
		    ExecuteJsonSet});
		registry.Add({"expect-json", Schema("pointer:string value:json"), "Assert the value and scalar type at a JSON pointer.", ScriptMode::Any, DecodeJsonSet,
		    ExecuteJsonSet});
		registry.Add({"expect-json-dirty", Schema("dirty:true|false"), "Assert whether the open JSON document has unsaved changes.", ScriptMode::Any,
		    DecodeExpectJsonDirty, ExecuteExpectJsonDirty});
		registry.Add({"json-save", Schema(""), "Save the open JSON document.", ScriptMode::Any, DecodeJsonSave, ExecuteJsonSave});
		registry.Add({"json-revert", Schema(""), "Discard edits and reload the open JSON document.", ScriptMode::Any, DecodeJsonRevert, ExecuteJsonRevert});
	}
} // namespace vega
