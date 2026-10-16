#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace vega
{
	struct EditorScriptCommand;
	struct EditorScriptParseResult;
	struct ScriptExecution;
	struct EditorContext;
	class CommandHistory;

	enum class ScriptMode
	{
		Any,
		Edit,
		Play
	};
	struct ScriptArgument
	{
		std::string name, type;
		bool optional = false;
		bool repeated = false;
	};
	struct ScriptArguments
	{
		// Both forms include the verb. JSON decoders need the original spelling.
		std::vector<std::string> tokens, rawTokens;
		bool Integer(std::size_t index, unsigned int& value) const;
		bool Real(std::size_t index, float& value) const;
	};
	struct ScriptSchema
	{
		std::vector<ScriptArgument> arguments;
		std::vector<std::size_t> counts;
		bool Validate(const ScriptArguments&) const;
		std::string Usage() const;
	};
	// name:type, [optional:type], value:type...; text is nonempty, string can be empty.
	// An explicit count set expresses paired optional arguments such as [x] [y].
	ScriptSchema Schema(const std::string& declaration, std::vector<std::size_t> counts = {});
	using ScriptDecoder = std::function<EditorScriptParseResult(const ScriptArguments&)>;
	using ScriptHandler = std::function<void(ScriptExecution&, const EditorScriptCommand&)>;
	struct ScriptCommandDefinition
	{
		std::string name;
		ScriptSchema schema;
		std::string help;
		ScriptMode mode = ScriptMode::Any;
		ScriptDecoder decode;
		ScriptHandler execute;
	};
	class ScriptCommandRegistry
	{
	public:
		void Add(ScriptCommandDefinition definition);
		const ScriptCommandDefinition* Find(const std::string& name) const;
		const std::map<std::string, ScriptCommandDefinition>& Commands() const { return mCommands; }
		std::string Help(const std::string& name = {}) const;
		std::string Markdown() const;

	private:
		std::map<std::string, ScriptCommandDefinition> mCommands;
	};
	void RegisterCoreScriptCommands(ScriptCommandRegistry&);
	using TokenScriptHandler = std::function<bool(EditorContext&, CommandHistory&, const std::vector<std::string>&, std::string&)>;
	ScriptCommandDefinition TokenScriptCommand(std::string name, ScriptSchema schema, std::string help, ScriptMode mode, TokenScriptHandler handler);
} // namespace vega
