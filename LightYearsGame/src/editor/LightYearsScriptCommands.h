#pragma once

#include <editor/ScriptCommandRegistry.h>

namespace ly
{
	// Share the declaration with the documentation check without starting game services.
	inline vega::ScriptCommandDefinition PlayerShipScriptCommand(vega::TokenScriptHandler handler)
	{
		return vega::TokenScriptCommand("expect-player-ship", vega::Schema("ship:text"), "Assert the player ship in the live Play world.",
										vega::ScriptMode::Play, std::move(handler));
	}
} // namespace ly
