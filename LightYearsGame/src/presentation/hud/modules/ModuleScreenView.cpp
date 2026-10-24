#include "presentation/hud/modules/ModuleScreenView.h"
#include "framework/MathUtility.h"
#include "presentation/hud/modules/ModuleCardTextLayout.h"
#include "presentation/hud/modules/HoloCanvas.h"
#include "presentation/hud/modules/HoloDraw.h"
#include "presentation/hud/modules/ShipHologramWidget.h"
#include "widget/UIStyle.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <SFML/System/String.hpp>

namespace vega
{
	namespace
	{
		constexpr float ScreenWidth = 1440.f;
		constexpr float ScreenHeight = 944.f;
		constexpr sf::Vector2f GridOrigin{ 24.f, 104.f };
		constexpr sf::Vector2f InventoryOrigin{ ModuleScreenView::CardOrigin.x, 224.f };
		constexpr float InventoryWidth = ModuleScreenView::CardExtent.x;
		constexpr sf::Vector2f SearchOrigin{ InventoryOrigin.x, 126.f };
		constexpr sf::Vector2f SearchSize{ 452.f, 28.f };
		constexpr sf::Vector2f ResetOrigin{ InventoryOrigin.x + 460.f, 126.f };
		constexpr float FilterY = 166.f;
		constexpr float FilterWidth = 176.f;
		constexpr float FilterPitch = 184.f;
		constexpr float FilterHeight = 28.f;

		void ClearLayer(Widget& root)
		{
			for (const auto& child : root.GetChildren()) child->DestroyWidget();
			root.NativeTick(0.f);
		}

		bool Contains(const sf::Vector2f& point, const sf::Vector2f& origin, const sf::Vector2f& size)
		{
			return point.x >= origin.x && point.y >= origin.y && point.x < origin.x + size.x && point.y < origin.y + size.y;
		}

		// zoneLevelBonus is filled only for installed entries, so cargo entries read as their plain level.
		std::string LevelText(const ModuleEntryView& entry)
		{
			return std::to_string(entry.level) + (entry.zoneLevelBonus > 0 ? " (+" + std::to_string(entry.zoneLevelBonus) + ")" : std::string{});
		}
		constexpr sf::Vector2f CardSize = ModuleScreenView::CardExtent;
		constexpr float DragThreshold = 12.f;
		// Display font matches the encounter banner and also serves as the HUD font (the kenvector face misreads letters at small sizes); module descriptions use a plain sans for reading.
		const std::string kTitleFont = ModuleCardText::TitleFont;
		const std::string kHudFont = kTitleFont;
		const std::string kBodyFont = ModuleCardText::BodyFont;

		// Holographic terminal palette, keyed off the HUD energy gauge cyan.
		const sf::Color Accent{ 70, 205, 255, 255 };
		const sf::Color AccentDim{ 70, 150, 200, 255 };
		const sf::Color Ink{ 226, 240, 255, 255 };
		const sf::Color InkMuted{ 132, 168, 196, 255 };
		const sf::Color Danger{ 255, 96, 96, 255 };
		const sf::Color Sync{ 110, 255, 170, 255 };
		const sf::Color SystemGold{ 255, 206, 92, 255 };

		sf::Color WithAlpha(sf::Color color, std::uint8_t alpha)
		{
			color.a = alpha;
			return color;
		}

		sf::Color Scale(const sf::Color& color, float factor)
		{
			const auto channel = [factor](std::uint8_t value) { return static_cast<std::uint8_t>(std::min(255.f, static_cast<float>(value) * factor)); };
			return { channel(color.r), channel(color.g), channel(color.b), color.a };
		}

		sf::Color ZoneColor(ModuleZoneKind zone)
		{
			switch (zone)
			{
			case ModuleZoneKind::Nose: return { 255, 84, 84, 255 };
			case ModuleZoneKind::Wings: return { 255, 160, 36, 255 };
			case ModuleZoneKind::Hull: return { 178, 200, 232, 255 };
			case ModuleZoneKind::Core: return { 60, 146, 255, 255 };
			case ModuleZoneKind::Engine: return { 60, 220, 120, 255 };
			case ModuleZoneKind::Sensor: return { 196, 110, 255, 255 };
			case ModuleZoneKind::AbilityBay: return { 255, 218, 56, 255 };
			case ModuleZoneKind::None: break;
			}
			return { 40, 42, 50, 255 };
		}

		// Modules take the color of their family, which matches the zone whose bonus they get.
		sf::Color FamilyColor(const std::string& family)
		{
			if (family == "Primary") return ZoneColor(ModuleZoneKind::Nose);
			if (family == "Offense") return ZoneColor(ModuleZoneKind::Wings);
			if (family == "Defense") return ZoneColor(ModuleZoneKind::Core);
			if (family == "Movement") return ZoneColor(ModuleZoneKind::Engine);
			if (family == "Utility") return ZoneColor(ModuleZoneKind::Sensor);
			if (family == "Ability") return ZoneColor(ModuleZoneKind::AbilityBay);
			return ZoneColor(ModuleZoneKind::Hull);
		}

		float Label(Widget& parent, const sf::Vector2f& position, const std::string& text, unsigned int size, const sf::Color& color, const std::string& font = kBodyFont)
		{
			if (text.empty()) return 0.f;
			auto child = parent.AddChild<TextWidget>(text, font, size);
			const auto widget = child.lock();
			if (!widget) return 0.f;
			widget->SetFillColor(color);
			widget->SetLayout(UILayout::Anchored(UIAnchor::TopLeft, position));
			return widget->GetBound().size.x;
		}

		// Label with a soft offset copy behind it so it glows against the plate.
		void GlowLabel(Widget& parent, const sf::Vector2f& position, const std::string& text, unsigned int size, const sf::Color& color, const sf::Color& glow, const std::string& font)
		{
			Label(parent, position + sf::Vector2f{ 1.5f, 1.5f }, text, size, WithAlpha(glow, static_cast<std::uint8_t>(glow.a * 0.55f)), font);
			Label(parent, position + sf::Vector2f{ -1.f, -0.5f }, text, size, WithAlpha(glow, static_cast<std::uint8_t>(glow.a * 0.35f)), font);
			Label(parent, position, text, size, color, font);
		}

		// Right-aligned label; returns its left edge.
		float LabelRight(Widget& parent, const sf::Vector2f& rightTop, const std::string& text, unsigned int size, const sf::Color& color, const std::string& font)
		{
			auto child = parent.AddChild<TextWidget>(text, font, size);
			const auto widget = child.lock();
			if (!widget) return rightTop.x;
			const float width = widget->GetBound().size.x;
			widget->SetFillColor(color);
			widget->SetLayout(UILayout::Anchored(UIAnchor::TopLeft, { rightTop.x - width, rightTop.y }));
			return rightTop.x - width;
		}

		std::string Upper(std::string text)
		{
			for (char& c : text) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
			return text;
		}

		std::string Lower(std::string text)
		{
			for (char& c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
			return text;
		}

		std::string Capitalized(std::string text)
		{
			if (!text.empty()) text[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(text[0])));
			return text;
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

		// Radial-ish vignette over a convex outline: rings shrink toward the center, colors follow the distance from it.
		void AddVignette(sf::VertexArray& va, const holo::Pts& outline, const sf::Vector2f& size)
		{
			const sf::Vector2f center = size * 0.5f;
			const sf::Color inside{ 9, 22, 40, 246 };
			const sf::Color edge{ 2, 5, 12, 250 };
			const auto colorAt = [&](const sf::Vector2f& p)
			{
				const float dx = (p.x - center.x) / center.x;
				const float dy = (p.y - center.y) / center.y;
				const float r = std::min(1.f, std::sqrt(dx * dx + dy * dy) / 1.25f);
				return holo::Lerp(inside, edge, r * r * (3.f - 2.f * r));
			};
			holo::Pts ring;
			for (std::size_t i = 0; i < outline.size(); ++i)
			{
				const sf::Vector2f a = outline[i];
				const sf::Vector2f b = outline[(i + 1) % outline.size()];
				const sf::Vector2f d = b - a;
				const int parts = std::max(1, static_cast<int>(std::sqrt(d.x * d.x + d.y * d.y) / 140.f));
				for (int k = 0; k < parts; ++k) ring.push_back(a + d * (static_cast<float>(k) / static_cast<float>(parts)));
			}
			const float scales[] = { 0.f, 0.4f, 0.7f, 0.88f, 1.f };
			for (int s = 0; s + 1 < 5; ++s)
				for (std::size_t i = 0; i < ring.size(); ++i)
				{
					const std::size_t j = (i + 1) % ring.size();
					const sf::Vector2f a0 = center + (ring[i] - center) * scales[s], b0 = center + (ring[j] - center) * scales[s];
					const sf::Vector2f a1 = center + (ring[i] - center) * scales[s + 1], b1 = center + (ring[j] - center) * scales[s + 1];
					if (s == 0) holo::Tri(va, center, a1, b1, colorAt(center), colorAt(a1), colorAt(b1));
					else holo::Quad(va, a0, b0, b1, a1, colorAt(a0), colorAt(b0), colorAt(b1), colorAt(a1));
				}
		}

		void AddSectionHeader(HoloCanvas& canvas, Widget& parent, const sf::Vector2f& position, const std::string& title)
		{
			holo::Diamond(canvas.Additive(), position + sf::Vector2f{ 4.f, 9.f }, 9.f, WithAlpha(Accent, 28));
			holo::Diamond(canvas.Additive(), position + sf::Vector2f{ 4.f, 9.f }, 4.f, Accent);
			Label(parent, position + sf::Vector2f{ 16.f, 0.f }, title, 13, Accent, kHudFont);
		}
	}

	ModuleScreenView::ModuleScreenView(EngineContext& context, shared_ptr<const ModuleScreenViewModel> viewModel, weak_ptr<ModuleScreenPresenter> presenter) : Panel{ context, { ScreenWidth, ScreenHeight } }, mViewModel{ std::move(viewModel) }, mPresenter{ std::move(presenter) }
	{
		SetBackgroundColor(sf::Color::Transparent);
		SetLayout(UILayout::Anchored(UIAnchor::Center, { 0.f, 0.f }, { ScreenWidth, ScreenHeight }));
		SetVisibility(false);

		// Static chrome: vignette, chamfered hairline frame, header rule, section headers and key hints.
		const auto chromeWeak = AddChild<HoloCanvas>();
		const auto chrome = chromeWeak.lock();
		if (chrome) chrome->SetLayout(UILayout::Stretch());
		if (chrome)
		{
			sf::VertexArray& normal = chrome->Normal();
			sf::VertexArray& add = chrome->Additive();
			const holo::Pts outline = holo::ChamferRect({ 0.f, 0.f }, { ScreenWidth, ScreenHeight }, 20.f, 8.f, 20.f, 8.f);
			AddVignette(normal, outline, { ScreenWidth, ScreenHeight });
			for (float y = 24.f; y < ScreenHeight - 24.f; y += 4.f) holo::Rect(add, { 0.f, y }, { ScreenWidth, 1.f }, WithAlpha(Accent, 4), WithAlpha(Accent, 4));
			holo::ClosedStrip(add, outline, -0.5f, 0.5f, WithAlpha(Accent, 120), WithAlpha(Accent, 120));
			holo::ClosedStrip(add, holo::ChamferRect({ 7.f, 7.f }, { ScreenWidth - 14.f, ScreenHeight - 14.f }, 15.f, 5.f, 15.f, 5.f), -0.5f, 0.5f, WithAlpha(Accent, 26), WithAlpha(Accent, 26));
			holo::Line(add, { 78.f, 0.5f }, { 236.f, 0.5f }, 3.f, Accent);
			holo::Line(add, { 78.f, 0.5f }, { 236.f, 0.5f }, 9.f, WithAlpha(Accent, 28));
			holo::LineGrad(add, { 40.f, 92.f }, { ScreenWidth - 40.f, 92.f }, 1.f, WithAlpha(Accent, 130), WithAlpha(Accent, 10));
			holo::Line(add, { 40.f, 92.f }, { 160.f, 92.f }, 3.f, Accent);
		}

		Label(*this, { 40.f, 26.f }, "MODULE BAY", 30, Ink, kTitleFont);
		Label(*this, { 42.f, 68.f }, "ENGINEERING  //  LOADOUT", 12, AccentDim, kHudFont);
		if (chrome)
		{
			AddSectionHeader(*chrome, *this, { 12.f, 97.f }, "SCHEMATIC");
			AddSectionHeader(*chrome, *this, { InventoryOrigin.x, 97.f }, "CARGO HOLD");
		}

		struct KeyHint { const char* key; const char* action; };
		const KeyHint hints[] = { { "TAB", "Close" }, { "LMB", "Pick up / place" }, { "RMB / T", "Rotate" }, { "DEL", "Uninstall" }, { "WHEEL", "Scroll cargo" } };
		float x = 40.f;
		const float y = ScreenHeight - 44.f;
		for (const KeyHint& hint : hints)
		{
			auto keyText = AddChild<TextWidget>(hint.key, kHudFont, 12);
			const auto keyWidget = keyText.lock();
			const float keyWidth = keyWidget ? keyWidget->GetBound().size.x : 30.f;
			if (keyWidget) keyWidget->DestroyWidget();
			if (chrome)
			{
				const holo::Pts cap = holo::ChamferRect({ x, y }, { keyWidth + 18.f, 26.f }, 0.f, 6.f, 0.f, 6.f);
				holo::FillConvex(chrome->Normal(), cap, sf::Color{ 8, 26, 44, 230 });
				holo::ClosedStrip(chrome->Additive(), cap, -0.5f, 0.5f, WithAlpha(Accent, 150), WithAlpha(Accent, 150));
			}
			Label(*this, { x + 9.f, y + 5.f }, hint.key, 12, Accent, kHudFont);
			x += keyWidth + 18.f + 10.f;
			x += Label(*this, { x, y + 2.f }, hint.action, 14, InkMuted) + 28.f;
		}

		const auto hologram = AddChild<ShipHologramWidget>();
		mGridArea = hologram;
		mInventoryArea = AddChild<Panel>();
		if (auto grid = mGridArea.lock()) grid->SetLayout(UILayout::Anchored(UIAnchor::TopLeft, GridOrigin));
		if (auto inventory = mInventoryArea.lock())
		{
			inventory->SetPanelSize({ InventoryWidth, InventoryVisibleRows * InventoryRowHeight });
			inventory->SetLayout(UILayout::Anchored(UIAnchor::TopLeft, InventoryOrigin));
		}
	}

	std::string ModuleScreenView::Wrapped(const std::string& text, unsigned int size, float width, unsigned int lines, const std::string& font)
	{
		const WrapKey key{ text, font, size, width, lines };
		const auto found = mWrappedText.find(key);
		if (found != mWrappedText.end()) return found->second;
		if (mWrappedText.size() >= 512) mWrappedText.clear();
		return mWrappedText.emplace(key, ModuleCardText::Wrap(text, font, size, width, lines).text).first->second;
	}

	void ModuleScreenView::Tick(float deltaTime)
	{
		Panel::Tick(deltaTime);
		if (!mViewModel) return;
		SetVisibility(mViewModel->isOpen);
		const bool changed = mWatcher.Consume(mViewModel->revision);
		const bool inventoryChanged = mInventoryWatcher.Consume(mViewModel->inventoryRevision);
		const bool labelsChanged = mLabelsWatcher.Consume(mViewModel->installedRevision);
		if (!mViewModel->isOpen)
		{
			mSearchFocused = false;
			mOpenFilter = -1;
			mDragArmed = false;
			mHoverCell.reset();
			mHoverInventoryId = 0;
			mForceRefresh = true;
			mLabelsWatcher.Reset();
			return;
		}
		if (changed || mForceRefresh || mForceScanRefresh || inventoryChanged || labelsChanged)
		{
			Refresh(inventoryChanged || mForceRefresh, labelsChanged);
			mForceRefresh = false;
			mForceScanRefresh = false;
		}
	}

	void ModuleScreenView::Refresh(bool refreshInventory, bool refreshLabels)
	{
		++mRefreshCount;
		const ModuleScreenViewModel& vm = *mViewModel;
		if (mDynamic.expired())
		{
			mDynamic = AddChild<Panel>();
			if (auto dynamic = mDynamic.lock())
			{
				dynamic->SetLayout(UILayout::Stretch());
				mInstalledLabels = dynamic->AddChild<Panel>();
				mScan = dynamic->AddChild<Panel>();
				mCargo = dynamic->AddChild<Panel>();
			}
			refreshInventory = true;
			refreshLabels = true;
		}
		if (refreshInventory) RebuildFilteredInventory();
		const auto root = mScan.lock();
		if (root) ClearLayer(*root);
		if (!root) return;
		root->SetLayout(UILayout::Stretch());
		const auto shapesWeak = root->AddChild<HoloCanvas>();
		const auto shapes = shapesWeak.lock();
		if (!shapes) return;
		shapes->SetLayout(UILayout::Stretch());
		sf::VertexArray& normal = shapes->Normal();
		sf::VertexArray& add = shapes->Additive();
		const auto hologram = std::static_pointer_cast<ShipHologramWidget>(mGridArea.lock());

		if (!vm.hasSchematic)
		{
			if (hologram) hologram->SetData(HologramData{});
			if (auto labels = mInstalledLabels.lock()) ClearLayer(*labels);
			if (auto cargo = mCargo.lock()) ClearLayer(*cargo);
			Label(*root, GridOrigin, "THIS SHIP HAS NO MODULE SLOTS", 16, InkMuted, kHudFont);
			return;
		}

		const float pitch = GridPitch(vm.gridWidth, vm.gridHeight);
		const sf::Vector2f gridSize{ vm.gridWidth * pitch, vm.gridHeight * pitch };
		// Narrow ships are centered in the ship area.
		const sf::Vector2f origin{ GridOrigin.x + std::max(0.f, (MaxGridWidth - gridSize.x) * 0.5f), GridOrigin.y + std::max(0.f, (MaxGridHeight - gridSize.y) * 0.5f) };
		if (auto grid = mGridArea.lock()) { grid->SetPanelSize(gridSize); grid->SetLayout(UILayout::Anchored(UIAnchor::TopLeft, origin)); }

		const ModuleEntryView* heldEntry = vm.heldInstanceId != 0 ? FindEntry(vm, vm.heldInstanceId) : nullptr;
		const ModuleEntryView* hoveredEntry = nullptr;
		if (!heldEntry && mHoverCell)
			for (const ModuleEntryView& entry : vm.installed)
				for (const sf::Vector2i& cell : entry.cells)
					if (cell == *mHoverCell) hoveredEntry = &entry;
		if (!heldEntry && !hoveredEntry && mHoverInventoryId != 0) hoveredEntry = FindEntry(vm, mHoverInventoryId);
		const ModuleEntryView* shown = heldEntry ? heldEntry : hoveredEntry;

		// Hologram data: slots, zone light, installed modules and the placement preview.
		if (hologram)
		{
			HologramData data;
			data.width = vm.gridWidth;
			data.height = vm.gridHeight;
			data.scale = pitch / CellPitch;
			const std::size_t count = static_cast<std::size_t>(vm.gridWidth * vm.gridHeight);
			data.zoneColors.assign(count, sf::Color::Transparent);
			data.active.assign(count, 0);
			data.engine.assign(count, 0);
			data.locked.assign(count, 0);
			for (std::size_t i = 0; i < count && i < vm.cellZones.size(); ++i)
			{
				const ModuleZoneKind zone = vm.cellZones[i];
				data.active[i] = zone != ModuleZoneKind::None ? 1 : 0;
				data.engine[i] = zone == ModuleZoneKind::Engine ? 1 : 0;
				data.zoneColors[i] = ZoneColor(zone);
				if (i < vm.cellLocked.size()) data.locked[i] = vm.cellLocked[i];
			}
			for (const ModuleEntryView& entry : vm.installed)
			{
				if (entry.cells.empty()) continue;
				HologramModule module;
				module.cells.assign(entry.cells.begin(), entry.cells.end());
				module.color = FamilyColor(entry.family);
				module.held = entry.held;
				module.rebooting = entry.rebooting;
				module.rebootFraction = entry.rebootFraction;
				module.hovered = &entry == hoveredEntry;
				module.system = entry.isSystem;
				data.modules.push_back(std::move(module));
			}
			data.previewActive = vm.preview.active;
			data.previewLegal = vm.preview.legal;
			data.previewCells.assign(vm.preview.cells.begin(), vm.preview.cells.end());
			hologram->SetData(data);
		}

		// Selection wash is independent of the cargo text/widgets.
		for (int row = 0; row < InventoryVisibleRows && mScroll + row < static_cast<int>(mFilteredInventory.size()); ++row)
		{
			const sf::Vector2f position{ InventoryOrigin.x, InventoryOrigin.y + row * InventoryRowHeight };
			const float rowHeight = InventoryRowHeight - 6.f;
			if (vm.inventory[mFilteredInventory[static_cast<std::size_t>(mScroll + row)]].held)
			{
				holo::RectH(normal, position, { InventoryWidth, rowHeight }, WithAlpha(Accent, 80), WithAlpha(Accent, 0));
				holo::LineGrad(add, position, position + sf::Vector2f{ InventoryWidth, 0.f }, 1.f, WithAlpha(Accent, 190), WithAlpha(Accent, 0));
				holo::LineGrad(add, position + sf::Vector2f{ 0.f, rowHeight }, position + sf::Vector2f{ InventoryWidth, rowHeight }, 1.f, WithAlpha(Accent, 190), WithAlpha(Accent, 0));
			}
		}

		// Scan readout: what you hold or hover, whether it fits, and what the zone gives it.
		// The scan card sits under the cargo list; the ship keeps the whole left column.
		const sf::Vector2f CardPosition = ModuleScreenView::CardOrigin;
		const std::string& problem = !vm.statusMessage.empty() ? vm.statusMessage : (vm.preview.active && !vm.preview.legal ? vm.preview.message : std::string{});
		const sf::Color cardAccent = !problem.empty() ? Danger : (shown ? FamilyColor(shown->family) : AccentDim);
		{
			const holo::Pts card = holo::ChamferRect(CardPosition, CardSize, 16.f, 0.f, 16.f, 0.f);
			holo::FillConvex(normal, card, sf::Color{ 6, 18, 34, 170 });
			holo::ClosedStrip(add, card, -0.5f, 0.5f, WithAlpha(cardAccent, 80), WithAlpha(cardAccent, 80));
			holo::Line(add, CardPosition + sf::Vector2f{ 18.f, 0.f }, CardPosition + sf::Vector2f{ 86.f, 0.f }, 2.f, cardAccent);
			holo::Line(add, CardPosition + CardSize - sf::Vector2f{ 86.f, 0.f }, CardPosition + CardSize - sf::Vector2f{ 18.f, 0.f }, 2.f, WithAlpha(cardAccent, 150));
			holo::RectH(add, CardPosition + sf::Vector2f{ 1.f, 1.f }, { 60.f, CardSize.y - 2.f }, WithAlpha(cardAccent, 22), WithAlpha(cardAccent, 0));
		}
		const sf::Vector2f in = CardPosition + sf::Vector2f{ 22.f, 12.f };
		const float right = CardPosition.x + CardSize.x - 22.f;
		if (!problem.empty())
		{
			Label(*root, in, "PLACEMENT BLOCKED", 12, Danger, kHudFont);
			Label(*root, in + sf::Vector2f{ 0.f, 26.f }, Wrapped(Capitalized(problem), 16, ModuleCardText::DetailWidth, 4), 16, Ink);
		}
		else if (shown)
		{
			Label(*root, in, heldEntry ? "HOLDING" : (!shown->cells.empty() ? "INSTALLED" : "CARGO"), 12, AccentDim, kHudFont);
			LabelRight(*root, { right, in.y - 2.f }, "LEVEL " + LevelText(*shown) + " / " + std::to_string(shown->levelCount), 13, InkMuted, kBodyFont);
			const int cells = shown->cellCount > 0 ? shown->cellCount : static_cast<int>(shown->cells.size());
			std::string detail = shown->rarity + "  /  " + shown->family + "  /  " + std::to_string(cells) + " cells";
			if (shown->sizeLadder.size() > 1)
			{
				detail += "  /  sizes ";
				for (std::size_t i = 0; i < shown->sizeLadder.size(); ++i) { if (i > 0) detail += " / "; const int size = shown->sizeLadder[i]; detail += size == cells ? "[" + std::to_string(size) + "]" : std::to_string(size); }
			}
			if (heldEntry) detail += "  /  " + std::to_string(vm.heldRotation * 90) + " deg";
			Label(*root, in + sf::Vector2f{ 0.f, 20.f }, Wrapped(Upper(shown->name), 16, ModuleCardText::DetailWidth, 1, kHudFont), 16, cardAccent, kHudFont);
			const bool hasBonus = heldEntry && vm.preview.active && vm.preview.legal && vm.preview.zoneLevelBonus > 0;
			Label(*root, in + sf::Vector2f{ 0.f, 42.f }, Wrapped(detail, 12, ModuleCardText::DetailWidth - (hasBonus ? 60.f : 0.f), 1), 12, InkMuted);
			if (heldEntry && vm.preview.active && vm.preview.legal && vm.preview.zoneLevelBonus > 0)
				LabelRight(*root, { right, in.y + 42.f }, "+" + std::to_string(vm.preview.zoneLevelBonus) + " Lv", 12, Sync, kBodyFont);
			Label(*root, in + sf::Vector2f{ 0.f, 62.f }, Wrapped(shown->description, 14, ModuleCardText::DetailWidth, 3), 14, Ink);
			std::string sideLabel = "NO SIDE FEATURE";
			sf::Color sideColor = InkMuted;
			if (shown->sideFeatureState == ModuleSideFeatureViewState::Unselected) sideLabel = "NO SIDE SELECTED  /  UNLOCKS AT " + std::to_string(shown->sideFeatureRequiredCells) + " CELLS";
			else if (shown->sideFeatureState != ModuleSideFeatureViewState::None)
			{
				sideLabel = shown->sideFeatureName + "  /  ";
				if (shown->sideFeatureState == ModuleSideFeatureViewState::Active) { sideLabel += shown->rebooting ? "REBOOTING" : (shown->cells.empty() ? "READY" : "ACTIVE"); sideColor = shown->rebooting ? InkMuted : Sync; }
				else if (shown->sideFeatureState == ModuleSideFeatureViewState::Locked) sideLabel += "LOCKED: " + std::to_string(shown->sideFeatureRequiredCells) + " CELLS";
				else sideLabel += "UNAVAILABLE";
			}
			Label(*root, in + sf::Vector2f{ 0.f, 124.f }, Wrapped(sideLabel, 12, ModuleCardText::DetailWidth, 1), 12, sideColor);
			Label(*root, in + sf::Vector2f{ 0.f, 145.f }, Wrapped(shown->sideFeatureDescription, 13, ModuleCardText::DetailWidth, 2), 13, shown->sideFeatureState == ModuleSideFeatureViewState::Active && !shown->rebooting ? Ink : InkMuted);
		}
		else
		{
			Label(*root, in, "MODULE SCAN", 12, AccentDim, kHudFont);
			Label(*root, in + sf::Vector2f{ 0.f, 20.f }, "NO MODULE SELECTED", 16, InkMuted, kHudFont);
			Label(*root, in + sf::Vector2f{ 0.f, 46.f }, "Hover a module to scan it. Pick one from the cargo hold\nor click an installed one to move it.", 14, InkMuted);
		}

		if (refreshLabels) RefreshInstalledLabels();
		if (refreshInventory) RefreshInventory();
	}

	void ModuleScreenView::RefreshInstalledLabels()
	{
		const ModuleScreenViewModel& vm = *mViewModel;
		const auto root = mInstalledLabels.lock();
		if (!root) return;
		ClearLayer(*root);
		root->SetLayout(UILayout::Stretch());
		const float pitch = GridPitch(vm.gridWidth, vm.gridHeight);
		const sf::Vector2f gridSize{ vm.gridWidth * pitch, vm.gridHeight * pitch };
		const sf::Vector2f origin{ GridOrigin.x + std::max(0.f, (MaxGridWidth - gridSize.x) * 0.5f), GridOrigin.y + std::max(0.f, (MaxGridHeight - gridSize.y) * 0.5f) };
		const auto cellPosition = [pitch, origin](int x, int y) { return sf::Vector2f{ origin.x + x * pitch, origin.y + y * pitch }; };
		// Labels sit on the hologram plates with a soft glow.
		for (const ModuleEntryView& entry : vm.installed)
		{
			if (entry.cells.empty()) continue;
			sf::Vector2i anchor = entry.cells.front();
			for (const sf::Vector2i& cell : entry.cells)
				if (cell.y < anchor.y || (cell.y == anchor.y && cell.x < anchor.x)) anchor = cell;
			const std::uint8_t alpha = entry.held ? std::uint8_t{ 70 } : std::uint8_t{ 255 };
			const sf::Color family = FamilyColor(entry.family);
			const sf::Vector2f first = cellPosition(anchor.x, anchor.y) + sf::Vector2f{ 3.f, 3.f };
			const sf::Color text = WithAlpha(holo::Lerp(family, sf::Color{ 255, 255, 255, 255 }, entry.isSystem ? 0.f : 0.6f), alpha);
			const float labelScale = pitch / CellPitch;
			GlowLabel(*root, first + sf::Vector2f{ 9.f, 11.f } * labelScale, entry.abbreviation, static_cast<unsigned int>(std::clamp(15.f * labelScale * 1.35f, 10.f, 15.f)), entry.isSystem ? WithAlpha(SystemGold, alpha) : text, WithAlpha(family, alpha), kHudFont);
			if (entry.rebooting) Label(*root, first + sf::Vector2f{ 9.f, 31.f }, FormatSeconds(entry.rebootRemaining), 12, Accent, kHudFont);
		}

	}

	void ModuleScreenView::RefreshInventory()
	{
		++mInventoryRefreshCount;
		const ModuleScreenViewModel& vm = *mViewModel;
		RebuildFilteredInventory();
		const auto root = mCargo.lock();
		if (!root) return;
		ClearLayer(*root);
		root->SetLayout(UILayout::Stretch());
		const auto shapes = root->AddChild<HoloCanvas>().lock();
		if (!shapes) return;
		shapes->SetLayout(UILayout::Stretch());
		sf::VertexArray& normal = shapes->Normal();
		sf::VertexArray& add = shapes->Additive();
		const sf::Vector2f CardPosition = ModuleScreenView::CardOrigin;
		// Cargo hold.
		const int total = static_cast<int>(mFilteredInventory.size());
		mScroll = std::clamp(mScroll, 0, std::max(0, total - InventoryVisibleRows));
		LabelRight(*root, { InventoryOrigin.x + InventoryWidth, 99.f }, std::to_string(total) + " / " + std::to_string(vm.inventory.size()) + " MODULES", 12, AccentDim, kHudFont);
		if (total == 0) Label(*root, InventoryOrigin + sf::Vector2f{ 16.f, 16.f }, vm.inventory.empty() ? "CARGO HOLD EMPTY" : "NO MATCHING MODULES", 14, InkMuted, kHudFont);
		for (int row = 0; row < InventoryVisibleRows && mScroll + row < total; ++row)
		{
			const ModuleEntryView& item = vm.inventory[mFilteredInventory[static_cast<std::size_t>(mScroll + row)]];
			const sf::Vector2f position{ InventoryOrigin.x, InventoryOrigin.y + row * InventoryRowHeight };
			const float rowHeight = InventoryRowHeight - 6.f;
			const sf::Color family = FamilyColor(item.family);
			holo::LineGrad(add, position + sf::Vector2f{ 0.f, rowHeight + 3.f }, position + sf::Vector2f{ InventoryWidth, rowHeight + 3.f }, 1.f, WithAlpha(Accent, 50), WithAlpha(Accent, 6));
			const sf::Vector2f mark = position + sf::Vector2f{ 9.f, rowHeight * 0.5f };
			holo::Diamond(add, mark, 11.f, WithAlpha(family, 34));
			holo::Diamond(add, mark, 5.f, family);
			const bool hasDescription = !item.description.empty();
			Label(*root, position + sf::Vector2f{ 26.f, hasDescription ? 7.f : 16.f }, Wrapped(Upper(item.name), 14, ModuleCardText::InventoryTextWidth, 1, kHudFont), 14, item.isSystem ? SystemGold : Ink, kHudFont);
			if (hasDescription) Label(*root, position + sf::Vector2f{ 26.f, 28.f }, Wrapped(item.description, 13, ModuleCardText::InventoryTextWidth, 1), 13, InkMuted);
			const float rowRight = position.x + InventoryWidth;
			Label(*root, { rowRight - 270.f, position.y + 7.f }, Upper(item.family), 11, family, kHudFont);
			LabelRight(*root, { position.x + InventoryWidth, position.y + 7.f }, "L" + LevelText(item), 11, InkMuted, kBodyFont);
			if (item.sideFeatureState != ModuleSideFeatureViewState::None)
				LabelRight(*root, { position.x + InventoryWidth, position.y + 29.f }, item.sideFeatureState == ModuleSideFeatureViewState::Active ? "+ SIDE" : (item.sideFeatureState == ModuleSideFeatureViewState::Locked ? "LOCKED" : "NO SIDE"), 10, item.sideFeatureState == ModuleSideFeatureViewState::Active ? Sync : InkMuted, kBodyFont);
			for (int cell = 0; cell < item.cellCount && cell < 8; ++cell)
			{
				const sf::Vector2f pip{ rowRight - 40.f - static_cast<float>(std::min(item.cellCount, 8) - cell) * 15.f + 5.f, position.y + 14.f };
				holo::Diamond(add, pip, 7.f, WithAlpha(family, 40));
				holo::Diamond(add, pip, 4.5f, family);
			}
		}
		if (total > InventoryVisibleRows)
		{
			const float track = InventoryVisibleRows * InventoryRowHeight - 6.f;
			const float thumb = std::max(28.f, track * static_cast<float>(InventoryVisibleRows) / static_cast<float>(total));
			const float offset = (track - thumb) * static_cast<float>(mScroll) / static_cast<float>(total - InventoryVisibleRows);
			const float barX = InventoryOrigin.x + InventoryWidth + 12.f;
			holo::Line(add, { barX, InventoryOrigin.y }, { barX, InventoryOrigin.y + track }, 1.f, WithAlpha(Accent, 40));
			holo::Line(add, { barX, InventoryOrigin.y + offset }, { barX, InventoryOrigin.y + offset + thumb }, 3.f, Accent);
		}

		// Zone key, under the cargo list.
		const float legendY = CardPosition.y + CardSize.y + 14.f;
		for (std::size_t i = 0; i < vm.legend.size(); ++i)
		{
			const sf::Vector2f position{ InventoryOrigin.x + static_cast<float>(i % 2) * 272.f, legendY + static_cast<float>(i / 2) * 26.f };
			const sf::Color zone = ZoneColor(vm.legend[i].zone);
			holo::Diamond(add, position + sf::Vector2f{ 6.f, 8.f }, 10.f, WithAlpha(zone, 34));
			holo::Diamond(add, position + sf::Vector2f{ 6.f, 8.f }, 5.f, zone);
			Label(*root, position + sf::Vector2f{ 18.f, 0.f }, Upper(vm.legend[i].label), 12, Scale(zone, 1.15f), kHudFont);
		}
		// Draw the filter plate after cargo labels so dropdowns fully cover the rows beneath them.
		if (auto filters = root->AddChild<HoloCanvas>().lock())
		{
			filters->SetLayout(UILayout::Stretch());
			DrawFilters(*root, *filters);
		}
	}

	List<std::string> ModuleScreenView::FilterOptions(int column) const
	{
		List<std::string> options{ "" };
		if (!mViewModel) return options;
		for (const ModuleEntryView& item : mViewModel->inventory)
		{
			const std::string value = column == 0 ? item.rarity : (column == 1 ? item.family : std::to_string(item.cellCount));
			if (!value.empty() && std::find(options.begin(), options.end(), value) == options.end()) options.push_back(value);
		}
		if (column == 2) std::sort(options.begin() + 1, options.end(), [](const std::string& a, const std::string& b) { return std::stoi(a) < std::stoi(b); });
		return options;
	}

	void ModuleScreenView::RebuildFilteredInventory()
	{
		mFilteredInventory.clear();
		if (!mViewModel) return;
		const std::string query = Lower(mSearch);
		for (std::size_t i = 0; i < mViewModel->inventory.size(); ++i)
		{
			const ModuleEntryView& item = mViewModel->inventory[i];
			if (!mFilters[0].empty() && item.rarity != mFilters[0]) continue;
			if (!mFilters[1].empty() && item.family != mFilters[1]) continue;
			if (!mFilters[2].empty() && std::to_string(item.cellCount) != mFilters[2]) continue;
			const std::string text = item.name + " " + item.abbreviation + " " + item.family + " " + item.rarity + " " + item.description + " " + item.sideFeatureName + " " + item.sideFeatureDescription;
			if (!query.empty() && Lower(text).find(query) == std::string::npos) continue;
			mFilteredInventory.push_back(i);
		}
		mScroll = std::clamp(mScroll, 0, std::max(0, static_cast<int>(mFilteredInventory.size()) - InventoryVisibleRows));
		if (mHoverInventoryId != 0 && std::none_of(mFilteredInventory.begin(), mFilteredInventory.end(), [&](std::size_t i) { return mViewModel->inventory[i].instanceId == mHoverInventoryId; })) mHoverInventoryId = 0;
	}

	void ModuleScreenView::FiltersChanged()
	{
		mScroll = 0;
		mHoverInventoryId = 0;
		mDragArmed = false;
		RebuildFilteredInventory();
		mForceRefresh = true;
	}

	void ModuleScreenView::DrawFilters(Widget& root, HoloCanvas& canvas)
	{
		const auto field = [&](const sf::Vector2f& origin, const sf::Vector2f& size, const std::string& text, bool active)
		{
			holo::Rect(canvas.Normal(), origin, size, sf::Color{ 8, 26, 44, 255 }, sf::Color{ 8, 26, 44, 255 });
			holo::ClosedStrip(canvas.Additive(), holo::ChamferRect(origin, size, 0.f, 4.f, 0.f, 4.f), -0.5f, 0.5f, active ? Accent : AccentDim, active ? Accent : AccentDim);
			Label(root, origin + sf::Vector2f{ 8.f, 5.f }, Wrapped(text, 13, size.x - 16.f, 1), 13, active ? Ink : InkMuted);
		};
		std::string searchText = mSearch.empty() ? "Search modules..." : mSearch;
		// Keep the caret and the latest typed text visible in a long query.
		while (!mSearch.empty() && ModuleCardText::Wrap(searchText + "|", kBodyFont, 13, SearchSize.x - 16.f, 1).truncated && !searchText.empty())
		{
			searchText.erase(0, 1);
			while (!searchText.empty() && (static_cast<unsigned char>(searchText.front()) & 0xc0) == 0x80) searchText.erase(0, 1);
		}
		if (mSearchFocused) searchText += "|";
		field(SearchOrigin, SearchSize, searchText, mSearchFocused);
		field(ResetOrigin, { 84.f, FilterHeight }, "Reset", false);
		const char* labels[] = { "Rarity", "Category", "Size" };
		for (int column = 0; column < 3; ++column)
		{
			const std::string value = mFilters[column].empty() ? "All" : mFilters[column] + (column == 2 ? " cells" : "");
			field({ InventoryOrigin.x + column * FilterPitch, FilterY }, { FilterWidth, FilterHeight }, std::string{ labels[column] } + ": " + value + " v", mOpenFilter == column || !mFilters[column].empty());
		}
		if (mOpenFilter < 0) return;
		const auto options = FilterOptions(mOpenFilter);
		for (std::size_t i = 0; i < options.size(); ++i)
			field({ InventoryOrigin.x + mOpenFilter * FilterPitch, FilterY + (i + 1) * FilterHeight }, { FilterWidth, FilterHeight }, options[i].empty() ? "All" : options[i] + (mOpenFilter == 2 ? " cells" : ""), mFilters[mOpenFilter] == options[i]);
	}

	sf::Vector2f ModuleScreenView::SearchFieldCenter() const
	{
		return GetResolvedRect().position + SearchOrigin + SearchSize * 0.5f;
	}

	sf::Vector2f ModuleScreenView::FilterControlCenter(int column) const
	{
		return GetResolvedRect().position + sf::Vector2f{ InventoryOrigin.x + column * FilterPitch + FilterWidth * 0.5f, FilterY + FilterHeight * 0.5f };
	}

	sf::Vector2f ModuleScreenView::FilterOptionCenter(int column, int option) const
	{
		return FilterControlCenter(column) + sf::Vector2f{ 0.f, (option + 1) * FilterHeight };
	}

	float ModuleScreenView::GridPitch(int gridWidth, int gridHeight)
	{
		if (gridWidth <= 0 || gridHeight <= 0) return CellPitch;
		return std::min({ CellPitch, MaxGridWidth / static_cast<float>(gridWidth), MaxGridHeight / static_cast<float>(gridHeight) });
	}

	std::optional<sf::Vector2i> ModuleScreenView::CellAt(const sf::Vector2f& point) const
	{
		const auto grid = mGridArea.lock();
		if (!mViewModel || !grid || !mViewModel->hasSchematic) return std::nullopt;
		const UIRect& rect = grid->GetResolvedRect();
		const float pitch = GridPitch(mViewModel->gridWidth, mViewModel->gridHeight);
		const int x = static_cast<int>(std::floor((point.x - rect.position.x) / pitch));
		const int y = static_cast<int>(std::floor((point.y - rect.position.y) / pitch));
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
		return row >= 0 && static_cast<std::size_t>(row) < mFilteredInventory.size() && mFilteredInventory[static_cast<std::size_t>(row)] < mViewModel->inventory.size() ? mViewModel->inventory[mFilteredInventory[static_cast<std::size_t>(row)]].instanceId : 0;
	}

	sf::Vector2f ModuleScreenView::CellCenter(const sf::Vector2i& cell) const
	{
		const auto grid = mGridArea.lock();
		const sf::Vector2f origin = grid ? grid->GetResolvedRect().position : sf::Vector2f{};
		const float pitch = mViewModel ? GridPitch(mViewModel->gridWidth, mViewModel->gridHeight) : CellPitch;
		return origin + sf::Vector2f{ (static_cast<float>(cell.x) + 0.5f) * pitch, (static_cast<float>(cell.y) + 0.5f) * pitch };
	}

	sf::Vector2f ModuleScreenView::InventoryRowCenter(int visibleRow) const
	{
		const auto inventory = mInventoryArea.lock();
		const UIRect rect = inventory ? inventory->GetResolvedRect() : UIRect{};
		return rect.position + sf::Vector2f{ rect.size.x * 0.5f, (static_cast<float>(visibleRow) + 0.5f) * InventoryRowHeight };
	}

	void ModuleScreenView::ScrollInventory(int rows)
	{
		const int total = static_cast<int>(mFilteredInventory.size());
		const int next = std::clamp(mScroll + rows, 0, std::max(0, total - InventoryVisibleRows));
		if (next == mScroll) return;
		mScroll = next;
		mForceRefresh = true;
	}

	bool ModuleScreenView::HandleEvent(const sf::Event& event)
	{
		const auto presenter = mPresenter.lock();
		if (!mViewModel || !mViewModel->isOpen || !presenter) return false;

		if (const auto* text = event.getIf<sf::Event::TextEntered>(); text && mSearchFocused)
		{
			if (text->unicode >= 32 && text->unicode != 127 && mSearch.size() < 128)
			{
				const auto utf8 = sf::String{ text->unicode }.toUtf8();
				mSearch.append(utf8.begin(), utf8.end());
				FiltersChanged();
			}
			return true;
		}
		if (const auto* key = event.getIf<sf::Event::KeyPressed>(); key && mSearchFocused)
		{
			if (key->code == sf::Keyboard::Key::Enter) { mSearchFocused = false; mForceRefresh = true; }
			else if (key->code == sf::Keyboard::Key::Backspace && !mSearch.empty())
			{
				std::size_t start = mSearch.size() - 1;
				while (start > 0 && (static_cast<unsigned char>(mSearch[start]) & 0xc0) == 0x80) --start;
				mSearch.erase(start);
				FiltersChanged();
			}
			else if (key->code == sf::Keyboard::Key::Delete) { mSearch.clear(); FiltersChanged(); }
			return true;
		}
		if (mSearchFocused && event.is<sf::Event::KeyReleased>()) return true;

		if (const auto* moved = event.getIf<sf::Event::MouseMoved>())
		{
			if (mOpenFilter >= 0) return true;
			const std::optional<sf::Vector2i> cell = CellAt({ static_cast<float>(moved->position.x), static_cast<float>(moved->position.y) });
			const auto inventoryId = InventoryItemAt({ static_cast<float>(moved->position.x), static_cast<float>(moved->position.y) });
			if (inventoryId != mHoverInventoryId) { mHoverInventoryId = inventoryId; mForceScanRefresh = true; }
			if (cell != mHoverCell)
			{
				mHoverCell = cell;
				mForceScanRefresh = true;
			}
			presenter->SetHoverCell(cell);
			return true;
		}
		if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>())
		{
			const sf::Vector2f point{ static_cast<float>(pressed->position.x), static_cast<float>(pressed->position.y) };
			const sf::Vector2f local = point - GetResolvedRect().position;
			if (pressed->button == sf::Mouse::Button::Left)
			{
				mDragArmed = false;
				const int previousFilter = mOpenFilter;
				if (mOpenFilter >= 0)
				{
					const auto options = FilterOptions(mOpenFilter);
					const sf::Vector2f origin{ InventoryOrigin.x + mOpenFilter * FilterPitch, FilterY + FilterHeight };
					if (Contains(local, origin, { FilterWidth, options.size() * FilterHeight }))
					{
						mFilters[mOpenFilter] = options[static_cast<std::size_t>((local.y - origin.y) / FilterHeight)];
						mOpenFilter = -1;
						FiltersChanged();
						return true;
					}
					mOpenFilter = -1;
					mForceRefresh = true;
				}
				const bool searchFocused = Contains(local, SearchOrigin, SearchSize);
				if (searchFocused != mSearchFocused) { mSearchFocused = searchFocused; mForceRefresh = true; }
				if (mSearchFocused) return true;
				if (Contains(local, ResetOrigin, { 84.f, FilterHeight }))
				{
					mSearch.clear();
					mFilters = {};
					FiltersChanged();
					return true;
				}
				for (int column = 0; column < 3; ++column)
					if (Contains(local, { InventoryOrigin.x + column * FilterPitch, FilterY }, { FilterWidth, FilterHeight }))
					{
						mOpenFilter = previousFilter == column ? -1 : column;
						mHoverInventoryId = 0;
						mForceRefresh = true;
						return true;
					}
			}
			else if (mOpenFilter >= 0 || mSearchFocused) return true;
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
			if (mOpenFilter >= 0) return true;
			const int direction = wheel->delta > 0.f ? 1 : -1;
			const sf::Vector2f point{ static_cast<float>(wheel->position.x), static_cast<float>(wheel->position.y) };
			if (IsOverInventory(point))
			{
				ScrollInventory(-direction);
				mHoverInventoryId = InventoryItemAt(point);
				mHoverCell.reset();
				presenter->SetHoverCell(std::nullopt);
				mForceRefresh = true;
			}
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
