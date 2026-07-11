#pragma once

#include "level/ArenaDefinition.h"
#include "framework/Actor.h"
#include <SFML/Graphics/RectangleShape.hpp>

namespace ly
{
	class ArenaBoundaryIndicator : public Actor
	{
	public:
		ArenaBoundaryIndicator(World* owningWorld, const ArenaDefinition& arenaDefinition);

		virtual void Render(sf::RenderWindow& window) override;

		void SetWarningActive(bool active);

	private:
		void RenderDebugRectangle(sf::RenderWindow& window);

		ArenaDefinition mArenaDefinition;
		bool mWarningActive;
	};
}

