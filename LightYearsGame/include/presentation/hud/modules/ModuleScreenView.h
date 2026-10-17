#pragma once

#include "presentation/hud/modules/ModuleScreenPresenter.h"
#include "widget/Panel.h"
#include "widget/TextWidget.h"
#include <optional>
#include <array>
#include <map>
#include <tuple>
#include "presentation/hud/modules/ModuleCardTextLayout.h"

namespace vega
{
	class HoloCanvas;
	// Module screen: schematic grid on the left, inventory list on the right. Mouse and key input is
	// translated into presenter intents; the view never touches gameplay state.
	class ModuleScreenView : public Panel
	{
	public:
		static constexpr float CellPitch = 64.f;
		static constexpr float MaxGridWidth = 800.f;
		static constexpr float MaxGridHeight = 680.f;
		// Cell size on screen: 64 for small grids, shrunk so any schematic fits the ship area.
		static float GridPitch(int gridWidth, int gridHeight);
		static constexpr float InventoryRowHeight = 58.f;
		static constexpr int InventoryVisibleRows = 6;
		// The scan card sits in the right column under the cargo list (the ship gets the whole left column).
		static constexpr sf::Vector2f CardOrigin{ 864.f, 604.f };
		static constexpr sf::Vector2f CardExtent{ 544.f, 194.f };

		ModuleScreenView(shared_ptr<const ModuleScreenViewModel> viewModel, weak_ptr<ModuleScreenPresenter> presenter);
		void Tick(float deltaTime) override;
		bool HandleEvent(const sf::Event& event) override;
		unsigned int GetRefreshCount() const { return mRefreshCount; }
		unsigned int GetInventoryRefreshCount() const { return mInventoryRefreshCount; }
		int GetInventoryScroll() const { return mScroll; }
		int GetFilteredInventoryCount() const { return static_cast<int>(mFilteredInventory.size()); }
		sf::Vector2f SearchFieldCenter() const;
		sf::Vector2f FilterControlCenter(int column) const;
		sf::Vector2f FilterOptionCenter(int column, int option) const;

		std::optional<sf::Vector2i> CellAt(const sf::Vector2f& point) const;
		bool IsOverInventory(const sf::Vector2f& point) const;
		// Instance id of the inventory row under the point, 0 when none.
		std::uint32_t InventoryItemAt(const sf::Vector2f& point) const;
		// Screen-space centers, valid after layout resolves; used by input and tests.
		sf::Vector2f CellCenter(const sf::Vector2i& cell) const;
		sf::Vector2f InventoryRowCenter(int visibleRow) const;

	private:
		void Refresh(bool refreshInventory, bool refreshLabels);
		void RefreshInventory();
		void RefreshInstalledLabels();
		std::string Wrapped(const std::string& text, unsigned int size, float width, unsigned int lines, const std::string& font = ModuleCardText::BodyFont);
		void ScrollInventory(int rows);
		void RebuildFilteredInventory();
		void FiltersChanged();
		List<std::string> FilterOptions(int column) const;
		void DrawFilters(Widget& root, HoloCanvas& canvas);

		shared_ptr<const ModuleScreenViewModel> mViewModel;
		weak_ptr<ModuleScreenPresenter> mPresenter;
		UIRevisionWatcher mWatcher, mInventoryWatcher, mLabelsWatcher;
		weak_ptr<Panel> mGridArea;
		weak_ptr<Panel> mInventoryArea;
		weak_ptr<Panel> mDynamic, mScan, mCargo, mInstalledLabels;
		sf::Vector2f mPressPosition{};
		std::optional<sf::Vector2i> mHoverCell;
		std::uint32_t mHoverInventoryId{ 0 };
		unsigned int mRefreshCount{ 0 }, mInventoryRefreshCount{ 0 };
		using WrapKey = std::tuple<std::string, std::string, unsigned int, float, unsigned int>;
		std::map<WrapKey, std::string> mWrappedText;
		int mScroll{ 0 };
		List<std::size_t> mFilteredInventory;
		std::array<std::string, 3> mFilters{};
		std::string mSearch;
		int mOpenFilter{ -1 };
		bool mSearchFocused{ false };
		bool mDragArmed{ false };
		bool mForceRefresh{ false };
		bool mForceScanRefresh{ false };
	};
}
