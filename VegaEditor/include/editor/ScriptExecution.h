#pragma once
#include "editor/EditorScript.h"
#include "editor/EditorCommands.h"

namespace vega
{
	struct ScriptExecution
	{
		EditorContext& context;
		CommandHistory& history;
		sf::RenderWindow& window;
		EditorScriptState& state;
		const ScriptCommandRegistry& registry;
		const std::string& line;
		bool success = false;
		std::string detail;
		bool deferReport = false;
	};
} // namespace vega
