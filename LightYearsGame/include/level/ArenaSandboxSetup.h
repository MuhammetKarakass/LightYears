#pragma once

#include "level/ArenaDefinition.h"
#include "player/PlayerRespawnDefinition.h"

namespace vega
{
	// Arena test and authored sandboxes share the same gameplay setup. Keep
	// these defaults together so scene loading cannot drift from arena play.
	ArenaDefinition CreateSandboxArenaDefinition();
	PlayerRespawnDefinition ConfigureSandboxPlayerRespawn(PlayerRespawnDefinition definition);
}
