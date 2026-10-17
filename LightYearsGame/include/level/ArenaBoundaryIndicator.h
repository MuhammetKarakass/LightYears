#pragma once

#include "level/ArenaDefinition.h"
#include "framework/Actor.h"
#include <SFML/Graphics/RectangleShape.hpp>

namespace vega
{
	class ArenaBoundaryIndicator : public Actor
	{
	public:
		ArenaBoundaryIndicator(World* owningWorld, const ArenaDefinition& arenaDefinition);

		virtual void Render(sf::RenderTarget& window) override;

		void SetWarningActive(bool active);

	private:
		void RenderDebugRectangle(sf::RenderTarget& window);

		ArenaDefinition mArenaDefinition;
		bool mWarningActive;
	};
}

