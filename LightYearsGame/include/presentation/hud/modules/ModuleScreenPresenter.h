#pragma once

#include "presentation/hud/modules/ModuleScreenViewModel.h"
#include <optional>
#include <string>
#include <map>
#include <tuple>

namespace ly
{
	class Player;
	struct ModuleDefinition;
	struct ModuleSizeVariant;
	struct ModuleSideFeatureDefinition;

	std::string FormatModuleDescription(const ModuleDefinition& definition, const ModuleSizeVariant& variant, int level = 1);
	std::string FormatModuleSideFeatureDescription(const ModuleSideFeatureDefinition& feature, int level);

	// Reads the player's loadout one way into the view model each Tick and turns UI intents into
	// Player::Try* / AddModuleToInventory calls. It never touches gameplay state any other way.
	class ModuleScreenPresenter
	{
	public:
		ModuleScreenPresenter();
		ModuleScreenPresenter(const ModuleScreenPresenter&) = delete;
		ModuleScreenPresenter& operator=(const ModuleScreenPresenter&) = delete;

		void Tick(float deltaTime);
		unsigned int GetContentBuildCount() const { return mContentBuildCount; }
		unsigned int GetDescriptionFormatCount() const { return mDescriptionFormatCount; }
		shared_ptr<const ModuleScreenViewModel> GetViewModel() const { return mViewModel; }

		void ToggleOpen();
		void SetOpen(bool open);
		// Cell under the cursor, or nullopt when the cursor is off the grid.
		void SetHoverCell(std::optional<sf::Vector2i> cell);
		void PickUpInventory(std::uint32_t instanceId);
		// Picks up the installed module covering the cell; false when the cell is empty.
		bool PickUpAtCell(sf::Vector2i cell);
		void CancelHold();
		void RotateHeld(int quarterTurns = 1);
		// Installs or moves the held module at the hovered cell. A failure keeps the hold and shows its text.
		bool DropOnGrid();
		// Dropping an installed module on the inventory uninstalls it; an inventory item just goes back.
		bool DropOnInventory();
		bool UninstallHeld();

	private:
		void Rebuild();
		void ShowFailure(const char* text);
		void ClearHold();

		shared_ptr<ModuleScreenViewModel> mViewModel;
		std::optional<sf::Vector2i> mHoverCell;
		std::uint32_t mHeldId{ 0 };
		int mHeldRotation{ 0 };
		std::string mMessage;
		float mMessageRemaining{ 0.f };
		bool mOpen{ false };
		bool mContentValid{ false };
		unsigned int mContentBuildCount{ 0 }, mDescriptionFormatCount{ 0 };
		unsigned int mPlayerId{ 0 }, mShipId{ 0 };
		std::uint64_t mLoadoutRevision{ 0 }, mCatalogRevision{ 0 };
		using DescriptionKey = std::tuple<std::string, int, int, std::string>;
		std::map<DescriptionKey, std::pair<std::string, std::string>> mDescriptions;
	};
}
