#pragma once

#include "framework/Core.h"
#include <SFML/System/Vector2.hpp>
#include <widget/UIViewModel.h>
#include <cstdint>
#include <string>

namespace ly
{
	enum class ModuleZoneKind { None, Nose, Wings, Hull, Core, Engine, Sensor };
	enum class ModuleSideFeatureViewState { None, Unselected, Locked, Active, Unavailable };

	// One installed placement or inventory item as the screen shows it.
	struct ModuleEntryView
	{
		std::uint32_t instanceId{ 0 };
		std::string name, abbreviation, family;
		std::string description;
		int level{ 1 }, levelCount{ 1 };
		List<int> sizeLadder;
		int sideFeatureRequiredCells{ 0 };
		ModuleSideFeatureViewState sideFeatureState{ ModuleSideFeatureViewState::None };
		std::string sideFeatureName, sideFeatureDescription;
		bool isSystem{ false };
		int cellCount{ 0 };
		int variantIndex{ 0 };
		// Covered grid cells; filled for installed modules only.
		List<sf::Vector2i> cells;
		int bonusPercent{ 0 };
		bool rebooting{ false };
		float rebootRemaining{ 0.f };
		// 0 just started, 1 finished.
		float rebootFraction{ 0.f };
		bool held{ false };

		bool operator==(const ModuleEntryView& o) const
		{
			return instanceId == o.instanceId && name == o.name && abbreviation == o.abbreviation && family == o.family && description == o.description && isSystem == o.isSystem &&
				level == o.level && levelCount == o.levelCount && sizeLadder == o.sizeLadder && sideFeatureRequiredCells == o.sideFeatureRequiredCells && sideFeatureState == o.sideFeatureState && sideFeatureName == o.sideFeatureName && sideFeatureDescription == o.sideFeatureDescription &&
				cellCount == o.cellCount && variantIndex == o.variantIndex && cells == o.cells && bonusPercent == o.bonusPercent &&
				rebooting == o.rebooting && rebootRemaining == o.rebootRemaining && rebootFraction == o.rebootFraction && held == o.held;
		}
	};

	struct ModuleLegendEntry
	{
		ModuleZoneKind zone{ ModuleZoneKind::None };
		std::string label;
		bool operator==(const ModuleLegendEntry& o) const { return zone == o.zone && label == o.label; }
	};

	// Placement preview for the held module at the hovered cell.
	struct ModulePreviewView
	{
		bool active{ false };
		List<sf::Vector2i> cells;
		bool legal{ false };
		// Zone bonus when legal; 0 means "no label" (never a warning).
		int bonusPercent{ 0 };
		std::string message;

		bool operator==(const ModulePreviewView& o) const
		{
			return active == o.active && cells == o.cells && legal == o.legal && bonusPercent == o.bonusPercent && message == o.message;
		}
	};

	struct ModuleScreenViewModel
	{
		bool isOpen{ false };
		bool hasSchematic{ false };
		int gridWidth{ 0 }, gridHeight{ 0 };
		// Row-major, gridWidth * gridHeight.
		List<ModuleZoneKind> cellZones;
		List<ModuleLegendEntry> legend;
		List<ModuleEntryView> installed;
		List<ModuleEntryView> inventory;
		ModulePreviewView preview;
		std::uint32_t heldInstanceId{ 0 };
		int heldRotation{ 0 };
		// Last command failure text, shown briefly.
		std::string statusMessage;
		UIRevision revision;
	};
}
