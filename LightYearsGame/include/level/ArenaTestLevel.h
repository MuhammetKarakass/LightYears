#pragma once

#include "level/ArenaLevel.h"

namespace ly
{
	class ArenaTestLevel : public ArenaLevel
	{
	public:
		ArenaTestLevel(Application* owningApp);

	protected:
		virtual ArenaDefinition CreateArenaDefinition() const override;
		virtual PlayerRespawnDefinition CreatePlayerRespawnDefinition() const override;
		virtual void OnGameStart() override;
	};
}
