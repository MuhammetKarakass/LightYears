#pragma once

#include "editor/EditorCommands.h"
#include <string>
#include <vector>

namespace vega
{
	// Runs a reflected CallInEditor function declared on the target's type. Catches any exception and
	// reports it like a refused property write: context.propertyError is filled for the Inspector and a
	// Console error is logged. Edit-mode-only functions are refused while playing. Calls are not undoable.
	bool CallReflectedFunction(EditorContext& context, const ReflectedTarget& target, unsigned int actorId,
		const std::string& functionName, std::string& error);

	// One CallInEditor action shared by every selected actor. targets holds one entry per selected actor,
	// in selection order, so a click runs the function on all of them.
	struct SelectionFunction
	{
		std::string section;
		std::string name;
		std::string label;
		std::string tooltip;
		bool callableInPlay = false;
		std::vector<ReflectedTarget> targets;
	};
	// Functions exposed by the matching target of every selected actor; empty for zero or one selection.
	std::vector<SelectionFunction> CommonSelectionFunctions(EditorContext& context);
	// Invokes row on every target, stopping at the first failure. The caller marks the scene after success.
	bool InvokeSelectionFunction(EditorContext& context, const SelectionFunction& row, std::string& error);
} // namespace vega
