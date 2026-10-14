#pragma once

#include "content/ContentId.h"
#include "framework/Core.h"

#include <SFML/System/Vector2.hpp>

#include <algorithm>

namespace ly
{
	// A module that comes installed on a ship. Origin is the grid cell the
	// rotated shape's (0,0) offset lands on.
	struct DefaultModulePlacement
	{
		sas::ContentId moduleId;
		int variantIndex = 0;
		sf::Vector2i origin;
		int rotationQuarterTurns = 0;
	};

	// Parsed ship grid. cellZones is row-major; an empty tag is "no cell".
	struct ShipSchematic
	{
		int width = 0;
		int height = 0;
		List<GameplayTag> cellZones;
		// Parallel to cellZones; a locked cell belongs to its zone but takes no modules yet. Empty means nothing is locked.
		List<char> cellLocked;
		List<DefaultModulePlacement> defaultModules;

		bool IsInBounds(int x, int y) const
		{
			return x >= 0 && y >= 0 && x < width && y < height;
		}

		bool IsLockedAt(int x, int y) const
		{
			return IsInBounds(x, y) && static_cast<std::size_t>(y * width + x) < cellLocked.size() && cellLocked[static_cast<std::size_t>(y * width + x)] != 0;
		}

		// Returns an empty tag outside the grid or on a void cell.
		const GameplayTag& GetZoneAt(int x, int y) const
		{
			static const GameplayTag none;
			return IsInBounds(x, y) ? cellZones[static_cast<std::size_t>(y * width + x)] : none;
		}
	};

	namespace ModuleShapeRules
	{
		// Each quarter turn maps (x, y) to (-y, x); the result is then shifted so
		// the minimum x and y are both 0. Quarter turns wrap modulo 4.
		inline List<sf::Vector2i> Rotate(const List<sf::Vector2i>& cells, int quarterTurns)
		{
			List<sf::Vector2i> rotated = cells;
			const int turns = ((quarterTurns % 4) + 4) % 4;
			for (int turn = 0; turn < turns; ++turn)
			{
				for (sf::Vector2i& cell : rotated)
				{
					cell = sf::Vector2i{ -cell.y, cell.x };
				}
			}
			if (rotated.empty())
			{
				return rotated;
			}
			int minX = rotated.front().x;
			int minY = rotated.front().y;
			for (const sf::Vector2i& cell : rotated)
			{
				minX = std::min(minX, cell.x);
				minY = std::min(minY, cell.y);
			}
			for (sf::Vector2i& cell : rotated)
			{
				cell = sf::Vector2i{ cell.x - minX, cell.y - minY };
			}
			return rotated;
		}
	}
}
