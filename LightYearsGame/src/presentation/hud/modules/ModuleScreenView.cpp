#include "presentation/hud/modules/ModuleScreenView.h"
#include "widget/UIStyle.h"
#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		constexpr float ScreenWidth = 860.f;
		constexpr float ScreenHeight = 620.f;
		constexpr sf::Vector2f GridOrigin{ 24.f, 64.f };
		constexpr sf::Vector2f InventoryOrigin{ 444.f, 64.f };
		constexpr float InventoryWidth = 392.f;
		constexpr float CellDraw = ModuleScreenView::CellPitch - 2.f;
		constexpr float DragThreshold = 12.f;

		sf::Color ZoneColor(ModuleZoneKind zone)
		{
			switch (zone)
			{
			case ModuleZoneKind::Nose: return { 196, 72, 72, 255 };
			case ModuleZoneKind::Wings: return { 214, 138, 52, 255 };
			case ModuleZoneKind::Hull: return { 108, 114, 126, 255 };
			case ModuleZoneKind::Core: return { 62, 118, 214, 255 };
			case ModuleZoneKind::Engine: return { 64, 170, 108, 255 };
			case ModuleZoneKind::Sensor: return { 156, 98, 204, 255 };
			case ModuleZoneKind::None: break;
			}
			return { 40, 42, 50, 255 };
		}

		void Rect(Widget& parent, const sf::Vector2f& position, const sf::Vector2f& size, const sf::Color& color)
		{
			auto child = parent.AddChild<Panel>(size);
			if (auto panel = child.lock())
			{
				panel->SetBackgroundColor(color);
				panel->SetLayout(UILayout::Anchored(UIAnchor::TopLeft, position, size));
			}
		}

		void Label(Widget& parent, const sf::Vector2f& position, const std::string& text, unsigned int size, const sf::Color& color)
		{
			if (text.empty()) return;
			auto child = parent.AddChild<TextWidget>(text, UIStyle::Get().Font(UIFontRole::Body), size);
			if (auto widget = child.lock())
			{
				widget->SetFillColor(color);
				widget->SetLayout(UILayout::Anchored(UIAnchor::TopLeft, position));
			}
		}

		std::string FormatSeconds(float seconds)
		{
			char buffer[16];
			snprintf(buffer, sizeof(buffer), "%.1fs", seconds);
			return buffer;
		}

		const ModuleEntryView* FindEntry(const ModuleScreenViewModel& vm, std::uint32_t instanceId)
		{
			for (const ModuleEntryView& entry : vm.installed)
				if (entry.instanceId == instanceId) return &entry;
			for (const ModuleEntryView& entry : vm.inventory)
				if (entry.instanceId == instanceId) return &entry;
			return nullptr;
		}
	}

	ModuleScreenView::ModuleScreenView(shared_ptr<const ModuleScreenViewModel> viewModel, weak_ptr<ModuleScreenPresenter> presenter)
		: Panel{ { ScreenWidth, ScreenHeight } }, mViewModel{ std::move(viewModel) }, mPresenter{ std::move(presenter) }
	{
		const UIStyle& style = UIStyle::Get();
		SetBackgroundColor(sf::Color{ 14, 16, 24, 235 });
		SetLayout(UILayout::Anchored(UIAnchor::Center, { 0.f, 0.f }, { ScreenWidth, ScreenHeight }));
		SetVisibility(false);
		Label(*this, { 24.f, 16.f }, "MODULES", style.TextSize(UITextSize::Large), style.Color(UIColorRole::Text));
		Label(*this, { 24.f, ScreenHeight - 26.f }, "Tab close   Click pick up / place   Right-click or T rotate   Del uninstall   Wheel scroll list", 12, style.Color(UIColorRole::TextMuted));
		mGridArea = AddChild<Panel>();
		mInventoryArea = AddChild<Panel>();
		if (auto grid = mGridArea.lock()) grid->SetLayout(UILayout::Anchored(UIAnchor::TopLeft, GridOrigin));
		if (auto inventory = mInventoryArea.lock())
		{
			inventory->SetPanelSize({ InventoryWidth, InventoryVisibleRows * InventoryRowHeight });
			inventory->SetLayout(UILayout::Anchored(UIAnchor::TopLeft, InventoryOrigin));
		}
	}

	void ModuleScreenView::Tick(float deltaTime)
	{
		Panel::Tick(deltaTime);
		if (!mViewModel) return;
		SetVisibility(mViewModel->isOpen);
		const bool changed = mWatcher.Consume(mViewModel->revision);
		if (!mViewModel->isOpen) return;
		if (changed || mForceRefresh)
		{
			mForceRefresh = false;
			Refresh();
		}
	}

	void ModuleScreenView::Refresh()
	{
		++mRefreshCount;
		const ModuleScreenViewModel& vm = *mViewModel;
		const UIStyle& style = UIStyle::Get();
		const sf::Color muted = style.Color(UIColorRole::TextMuted);
		const sf::Color text = style.Color(UIColorRole::Text);
		if (auto old = mDynamic.lock()) old->DestroyWidget();
		mDynamic = AddChild<Panel>();
		const auto root = mDynamic.lock();
		if (!root) return;
		root->SetLayout(UILayout::Stretch());

		if (!vm.hasSchematic)
		{
			Label(*root, GridOrigin, "This ship has no module slots.", style.TextSize(UITextSize::Body), muted);
			return;
		}

		const sf::Vector2f gridSize{ vm.gridWidth * CellPitch, vm.gridHeight * CellPitch };
		if (auto grid = mGridArea.lock()) grid->SetPanelSize(gridSize);
		const auto cellPosition = [](int x, int y) { return sf::Vector2f{ GridOrigin.x + x * CellPitch, GridOrigin.y + y * CellPitch }; };

		for (int y = 0; y < vm.gridHeight; ++y)
			for (int x = 0; x < vm.gridWidth; ++x)
			{
				const ModuleZoneKind zone = vm.cellZones[static_cast<std::size_t>(y * vm.gridWidth + x)];
				if (zone != ModuleZoneKind::None) Rect(*root, cellPosition(x, y), { CellDraw, CellDraw }, ZoneColor(zone));
			}

		for (const ModuleEntryView& entry : vm.installed)
		{
			if (entry.cells.empty()) continue;
			const sf::Color fill = entry.isSystem ? sf::Color{ 255, 204, 64, entry.held ? std::uint8_t{ 90 } : std::uint8_t{ 245 } }
				: sf::Color{ 228, 231, 240, entry.held ? std::uint8_t{ 90 } : std::uint8_t{ 235 } };
			for (const sf::Vector2i& cell : entry.cells)
			{
				Rect(*root, cellPosition(cell.x, cell.y) + sf::Vector2f{ 3.f, 3.f }, { CellDraw - 6.f, CellDraw - 6.f }, fill);
				if (entry.rebooting) Rect(*root, cellPosition(cell.x, cell.y) + sf::Vector2f{ 3.f, 3.f }, { CellDraw - 6.f, CellDraw - 6.f }, sf::Color{ 10, 10, 16, 175 });
			}
			const sf::Vector2f first = cellPosition(entry.cells.front().x, entry.cells.front().y);
			Label(*root, first + sf::Vector2f{ 7.f, 7.f }, entry.abbreviation, 14, sf::Color{ 20, 22, 30, 255 });
			if (entry.rebooting)
			{
				Label(*root, first + sf::Vector2f{ 7.f, 24.f }, FormatSeconds(entry.rebootRemaining), 11, sf::Color{ 140, 230, 255, 255 });
				Rect(*root, first + sf::Vector2f{ 3.f, CellDraw - 8.f }, { (CellDraw - 6.f) * entry.rebootFraction, 4.f }, sf::Color{ 90, 210, 255, 255 });
			}
		}

		if (vm.preview.active)
		{
			const sf::Color tint = vm.preview.legal ? sf::Color{ 80, 235, 110, 150 } : sf::Color{ 240, 70, 70, 150 };
			for (const sf::Vector2i& cell : vm.preview.cells)
				if (cell.x >= 0 && cell.y >= 0 && cell.x < vm.gridWidth && cell.y < vm.gridHeight) Rect(*root, cellPosition(cell.x, cell.y), { CellDraw, CellDraw }, tint);
		}

		const float infoY = GridOrigin.y + gridSize.y + 8.f;
		const ModuleEntryView* heldEntry = vm.heldInstanceId != 0 ? FindEntry(vm, vm.heldInstanceId) : nullptr;
		if (!vm.statusMessage.empty()) Label(*root, { GridOrigin.x, infoY }, vm.statusMessage, style.TextSize(UITextSize::Body), style.Color(UIColorRole::Danger));
		else if (vm.preview.active && !vm.preview.legal) Label(*root, { GridOrigin.x, infoY }, vm.preview.message, style.TextSize(UITextSize::Body), style.Color(UIColorRole::Danger));
		else if (vm.preview.active && vm.preview.bonusPercent > 0) Label(*root, { GridOrigin.x, infoY }, "+" + std::to_string(vm.preview.bonusPercent) + "% zone bonus", style.TextSize(UITextSize::Body), style.Color(UIColorRole::Positive));
		else if (heldEntry) Label(*root, { GridOrigin.x, infoY }, "Holding " + heldEntry->name + "  (" + std::to_string(vm.heldRotation * 90) + " deg)", style.TextSize(UITextSize::Body), text);
		else Label(*root, { GridOrigin.x, infoY }, "Pick a module from the list or click an installed one.", style.TextSize(UITextSize::Body), muted);

		const float legendY = infoY + 36.f;
		for (std::size_t i = 0; i < vm.legend.size(); ++i)
		{
			const sf::Vector2f position{ GridOrigin.x + static_cast<float>(i % 2) * 200.f, legendY + static_cast<float>(i / 2) * 22.f };
			Rect(*root, position, { 14.f, 14.f }, ZoneColor(vm.legend[i].zone));
			Label(*root, position + sf::Vector2f{ 20.f, 0.f }, vm.legend[i].label, 12, text);
		}

		const int total = static_cast<int>(vm.inventory.size());
		mScroll = std::clamp(mScroll, 0, std::max(0, total - InventoryVisibleRows));
		Label(*root, { InventoryOrigin.x, 40.f }, "INVENTORY (" + std::to_string(total) + ")", style.TextSize(UITextSize::Body), text);
		if (total == 0) Label(*root, InventoryOrigin, "No uninstalled modules.", style.TextSize(UITextSize::Body), muted);
		for (int row = 0; row < InventoryVisibleRows && mScroll + row < total; ++row)
		{
			const ModuleEntryView& item = vm.inventory[static_cast<std::size_t>(mScroll + row)];
			const sf::Vector2f position{ InventoryOrigin.x, InventoryOrigin.y + row * InventoryRowHeight };
			Rect(*root, position, { InventoryWidth, InventoryRowHeight - 2.f }, item.held ? sf::Color{ 58, 96, 160, 255 } : sf::Color{ 30, 33, 44, 255 });
			Label(*root, position + sf::Vector2f{ 8.f, 5.f }, item.name + "  " + item.family + "  " + std::to_string(item.cellCount) + " cells", 13, item.isSystem ? sf::Color{ 255, 214, 100, 255 } : text);
		}
	}

	std::optional<sf::Vector2i> ModuleScreenView::CellAt(const sf::Vector2f& point) const
	{
		const auto grid = mGridArea.lock();
		if (!mViewModel || !grid || !mViewModel->hasSchematic) return std::nullopt;
		const UIRect& rect = grid->GetResolvedRect();
		const int x = static_cast<int>(std::floor((point.x - rect.position.x) / CellPitch));
		const int y = static_cast<int>(std::floor((point.y - rect.position.y) / CellPitch));
		if (point.x < rect.position.x || point.y < rect.position.y || x >= mViewModel->gridWidth || y >= mViewModel->gridHeight) return std::nullopt;
		return sf::Vector2i{ x, y };
	}

	bool ModuleScreenView::IsOverInventory(const sf::Vector2f& point) const
	{
		const auto inventory = mInventoryArea.lock();
		if (!inventory) return false;
		const UIRect& rect = inventory->GetResolvedRect();
		return point.x >= rect.position.x && point.y >= rect.position.y && point.x < rect.position.x + rect.size.x && point.y < rect.position.y + rect.size.y;
	}

	std::uint32_t ModuleScreenView::InventoryItemAt(const sf::Vector2f& point) const
	{
		const auto inventory = mInventoryArea.lock();
		if (!mViewModel || !inventory || !IsOverInventory(point)) return 0;
		const int row = mScroll + static_cast<int>((point.y - inventory->GetResolvedRect().position.y) / InventoryRowHeight);
		return row >= 0 && static_cast<std::size_t>(row) < mViewModel->inventory.size() ? mViewModel->inventory[static_cast<std::size_t>(row)].instanceId : 0;
	}

	sf::Vector2f ModuleScreenView::CellCenter(const sf::Vector2i& cell) const
	{
		const auto grid = mGridArea.lock();
		const sf::Vector2f origin = grid ? grid->GetResolvedRect().position : sf::Vector2f{};
		return origin + sf::Vector2f{ (static_cast<float>(cell.x) + 0.5f) * CellPitch, (static_cast<float>(cell.y) + 0.5f) * CellPitch };
	}

	sf::Vector2f ModuleScreenView::InventoryRowCenter(int visibleRow) const
	{
		const auto inventory = mInventoryArea.lock();
		const UIRect rect = inventory ? inventory->GetResolvedRect() : UIRect{};
		return rect.position + sf::Vector2f{ rect.size.x * 0.5f, (static_cast<float>(visibleRow) + 0.5f) * InventoryRowHeight };
	}

	void ModuleScreenView::ScrollInventory(int rows)
	{
		const int total = mViewModel ? static_cast<int>(mViewModel->inventory.size()) : 0;
		const int next = std::clamp(mScroll + rows, 0, std::max(0, total - InventoryVisibleRows));
		if (next == mScroll) return;
		mScroll = next;
		mForceRefresh = true;
	}

	bool ModuleScreenView::HandleEvent(const sf::Event& event)
	{
		const auto presenter = mPresenter.lock();
		if (!mViewModel || !mViewModel->isOpen || !presenter) return false;

		if (const auto* moved = event.getIf<sf::Event::MouseMoved>())
		{
			presenter->SetHoverCell(CellAt({ static_cast<float>(moved->position.x), static_cast<float>(moved->position.y) }));
			return true;
		}
		if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>())
		{
			const sf::Vector2f point{ static_cast<float>(pressed->position.x), static_cast<float>(pressed->position.y) };
			if (pressed->button == sf::Mouse::Button::Right)
			{
				presenter->RotateHeld();
				return true;
			}
			if (pressed->button != sf::Mouse::Button::Left) return true;
			const std::optional<sf::Vector2i> cell = CellAt(point);
			if (mViewModel->heldInstanceId != 0)
			{
				if (cell)
				{
					presenter->SetHoverCell(cell);
					presenter->DropOnGrid();
				}
				else if (IsOverInventory(point)) presenter->DropOnInventory();
				return true;
			}
			if (cell) mDragArmed = presenter->PickUpAtCell(*cell);
			else if (const std::uint32_t item = InventoryItemAt(point); item != 0)
			{
				presenter->PickUpInventory(item);
				mDragArmed = true;
			}
			mPressPosition = point;
			return true;
		}
		if (const auto* released = event.getIf<sf::Event::MouseButtonReleased>())
		{
			if (released->button != sf::Mouse::Button::Left) return true;
			const sf::Vector2f point{ static_cast<float>(released->position.x), static_cast<float>(released->position.y) };
			const sf::Vector2f delta = point - mPressPosition;
			const bool dragged = mDragArmed && std::sqrt(delta.x * delta.x + delta.y * delta.y) > DragThreshold;
			mDragArmed = false;
			if (!dragged) return true;
			if (const std::optional<sf::Vector2i> cell = CellAt(point))
			{
				presenter->SetHoverCell(cell);
				presenter->DropOnGrid();
			}
			else if (IsOverInventory(point)) presenter->DropOnInventory();
			return true;
		}
		if (const auto* wheel = event.getIf<sf::Event::MouseWheelScrolled>())
		{
			const int direction = wheel->delta > 0.f ? 1 : -1;
			if (IsOverInventory({ static_cast<float>(wheel->position.x), static_cast<float>(wheel->position.y) })) ScrollInventory(-direction);
			else presenter->RotateHeld(direction);
			return true;
		}
		if (const auto* key = event.getIf<sf::Event::KeyPressed>())
		{
			if (key->code == sf::Keyboard::Key::T)
			{
				presenter->RotateHeld();
				return true;
			}
			if (key->code == sf::Keyboard::Key::Delete || key->code == sf::Keyboard::Key::Backspace)
			{
				presenter->UninstallHeld();
				return true;
			}
		}
		return false;
	}
}
