#pragma once

#include "presentation/hud/modules/ModuleScreenPresenter.h"
#include "widget/Panel.h"
#include "widget/TextWidget.h"
#include <optional>

namespace ly
{
	// Module screen: schematic grid on the left, inventory list on the right. Mouse and key input is
	// translated into presenter intents; the view never touches gameplay state.
	class ModuleScreenView : public Panel
	{
	public:
		static constexpr float CellPitch = 48.f;
		static constexpr float InventoryRowHeight = 32.f;
		static constexpr int InventoryVisibleRows = 16;

		ModuleScreenView(shared_ptr<const ModuleScreenViewModel> viewModel, weak_ptr<ModuleScreenPresenter> presenter);
		void Tick(float deltaTime) override;
		bool HandleEvent(const sf::Event& event) override;
		unsigned int GetRefreshCount() const { return mRefreshCount; }
		int GetInventoryScroll() const { return mScroll; }

		std::optional<sf::Vector2i> CellAt(const sf::Vector2f& point) const;
		bool IsOverInventory(const sf::Vector2f& point) const;
		// Instance id of the inventory row under the point, 0 when none.
		std::uint32_t InventoryItemAt(const sf::Vector2f& point) const;
		// Screen-space centers, valid after layout resolves; used by input and tests.
		sf::Vector2f CellCenter(const sf::Vector2i& cell) const;
		sf::Vector2f InventoryRowCenter(int visibleRow) const;

	private:
		void Refresh();
		void ScrollInventory(int rows);

		shared_ptr<const ModuleScreenViewModel> mViewModel;
		weak_ptr<ModuleScreenPresenter> mPresenter;
		UIRevisionWatcher mWatcher;
		weak_ptr<Panel> mGridArea;
		weak_ptr<Panel> mInventoryArea;
		weak_ptr<Panel> mDynamic;
		sf::Vector2f mPressPosition{};
		unsigned int mRefreshCount{ 0 };
		int mScroll{ 0 };
		bool mDragArmed{ false };
		bool mForceRefresh{ false };
	};
}
