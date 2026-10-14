#include "CommonModuleE2ETestSupport.h"
#include "gameplay/content/ModuleSideFeatureLoader.h"
#include "player/Player.h"
#include "player/PlayerManager.h"
#include "presentation/hud/modules/ModuleScreenView.h"
#include "presentation/hud/modules/ModuleCardTextLayout.h"
#include <SFML/Graphics/Texture.hpp>
#include <cctype>
#include <sstream>

namespace
{
	using namespace ly;
	using common_module_e2e::Report;
	using common_module_e2e::Json;
	struct ResetGuard { ~ResetGuard() { PlayerManager::GetPlayerManager().Reset(); } };

	const ModuleEntryView& Entry(const ModuleScreenViewModel& model, std::uint32_t id)
	{
		for (const auto& entry : model.inventory) if (entry.instanceId == id) return entry;
		for (const auto& entry : model.installed) if (entry.instanceId == id) return entry;
		throw std::runtime_error{ "Missing projected module " + std::to_string(id) };
	}

	void CheckMissingFontFallback(Report& report)
	{
		const auto layout = ModuleCardText::Wrap("Primary hits add Energy damage for a few seconds after the shield breaks and cannot critically hit.", "fonts/DoesNotExist.ttf", 14, 220.f, 2);
		report.Check(!layout.text.empty() && layout.lineCount >= 1 && layout.lineCount <= 2 && layout.truncated, "card text wraps approximately instead of throwing when the font is missing", layout.text);
	}

	void Measure(Report& report, const std::string& text, unsigned int size, float width, unsigned int lines, const std::string& name, bool requireComplete, const std::string& fontPath = ModuleCardText::BodyFont)
	{
		const auto layout = ModuleCardText::Wrap(text, fontPath, size, width, lines);
		const auto font = AssetManager::GetAssetManager().LoadFont(fontPath);
		sf::Text measured{ *font, "", size };
		std::istringstream stream{ layout.text };
		std::string line;
		float widest = 0.f;
		while (std::getline(stream, line)) { measured.setString(sf::String::fromUtf8(line.begin(), line.end())); widest = std::max(widest, measured.getLocalBounds().size.x); }
		report.Check(widest <= width && layout.lineCount <= lines && (!requireComplete || !layout.truncated), name,
			{ { "text", layout.text }, { "width", widest }, { "limit", width }, { "lines", layout.lineCount }, { "truncated", layout.truncated } });
	}

	std::optional<ModulePlacement> Place(const ModuleRulesContext& rules, const ModuleLoadout& loadout, std::uint32_t id, std::uint32_t ignored = 0, std::optional<sf::Vector2i> excluded = std::nullopt)
	{
		const auto* item = loadout.FindInventoryItem(id);
		if (!item) item = loadout.FindPlacement(id);
		if (!item) return std::nullopt;
		for (int rotation = 0; rotation < 4; ++rotation)
			for (int y = 0; y < rules.schematic.height; ++y)
				for (int x = 0; x < rules.schematic.width; ++x)
				{
					auto candidate = *item; candidate.origin = { x, y }; candidate.rotationQuarterTurns = rotation;
					if (excluded && candidate.origin == *excluded) continue;
					if (ModulePlacementRules::CanPlace(rules, loadout, candidate, ignored) == ModulePlacementFailure::None) return candidate;
				}
		return std::nullopt;
	}

	void Screenshot(Report& report, ModuleScreenView& view, sf::RenderWindow& window, const std::filesystem::path& path)
	{
		window.clear(sf::Color{ 2, 5, 12 }); view.NativeDraw(window); window.display();
		sf::Texture captured; report.Check(captured.resize(window.getSize()), "capture texture resized"); captured.update(window);
		report.Check(captured.copyToImage().saveToFile(path), "real module screen saved", path.generic_string());
	}

	void CardBounds(Report& report, const Widget& widget, const sf::Vector2f& origin, int& count)
	{
		if (dynamic_cast<const TextWidget*>(&widget))
		{
			const auto bound = widget.GetBound(); const auto p = bound.position - origin;
			const sf::Vector2f cardOrigin = ModuleScreenView::CardOrigin, cardExtent = ModuleScreenView::CardExtent;
			if (p.x >= cardOrigin.x && p.x < cardOrigin.x + cardExtent.x && p.y >= cardOrigin.y + 10.f && p.y < cardOrigin.y + cardExtent.y + 2.f)
			{
				++count;
				report.Check(p.x >= cardOrigin.x + 22.f && p.x + bound.size.x <= cardOrigin.x + cardExtent.x - 22.f + 0.1f && p.y + bound.size.y < cardOrigin.y + cardExtent.y + 2.f, "actual card TextWidget bounds",
					{ { "x", p.x }, { "y", p.y }, { "width", bound.size.x }, { "height", bound.size.y } });
			}
		}
		for (const auto& child : widget.GetChildren()) CardBounds(report, *child, origin, count);
	}

	void Run(Report& report, Application& application, const std::filesystem::path& artifacts)
	{
		CheckMissingFontFallback(report);
		auto& manager = PlayerManager::GetPlayerManager(); manager.Reset(); ResetGuard reset;
		auto level = std::make_shared<common_module_e2e::HeadlessModuleLevel>(&application);
		auto& player = manager.CreateNewPlayer(); auto ship = player.SpawnSpaceShip(level.get()).lock();
		report.Check(static_cast<bool>(ship), "real player ship spawned"); level->BeginPlayInternal(); level->TickInternal(0.f);
		auto presenter = std::make_shared<ModuleScreenPresenter>(); presenter->SetOpen(true); presenter->Tick(0.f);
		const auto model = presenter->GetViewModel(); const auto rules = player.BuildModuleRules();
		report.Check(rules.has_value() && model->hasSchematic, "real schematic and presenter available");
		ModuleScreenView view{ model, presenter };
		const auto tick = [&]() { level->TickInternal(0.f); presenter->Tick(0.f); view.NativeTick(0.f); view.ResolveLayoutTree({ { 0.f, 0.f }, { 1920.f, 1080.f } }, true); };
		const auto point = [](sf::Vector2f p) { return sf::Vector2i{ static_cast<int>(p.x), static_cast<int>(p.y) }; };
		std::uint32_t ricochetSmall = 0, ricochetLarge = 0, trailCard = 0, gritCard = 0;
		int modules = 0, combinations = 0;
		for (const auto& definition : content::ModuleContentCatalog::GetDefinitions())
		{
			if (definition.rarity != ModuleRarity::Uncommon) continue;
			++modules;
			List<sas::ContentId> sides{ sas::ContentId{} };
			for (const auto& id : definition.sideFeatures) sides.push_back(id);
			for (const auto& pool : definition.sharedPools) for (const auto& id : pool.allowedFeatureIds) sides.push_back(id);
			List<int> ladder; for (const auto& variant : definition.sizeVariants) ladder.push_back(static_cast<int>(variant.shape.cells.size()));
			std::sort(ladder.begin(), ladder.end()); ladder.erase(std::unique(ladder.begin(), ladder.end()), ladder.end());
			for (std::size_t variant = 0; variant < definition.sizeVariants.size(); ++variant)
				for (int levelIndex = 1; levelIndex <= definition.levelCount; ++levelIndex)
					for (const auto& side : sides)
					{
						const std::string name = definition.moduleId.ToString() + "/v" + std::to_string(variant) + "/L" + std::to_string(levelIndex) + "/" + side.ToString();
						const auto id = player.AddModuleToInventory(definition.moduleId, static_cast<int>(variant), ModuleRarity::Uncommon, levelIndex, side);
						report.Check(id != 0, name + " real inventory accepts combination"); presenter->Tick(0.f); ++combinations;
						const auto& entry = Entry(*model, id);
						report.Check(entry.level == levelIndex && entry.levelCount == definition.levelCount && entry.sizeLadder == ladder && entry.cellCount == static_cast<int>(definition.sizeVariants[variant].shape.cells.size()), name + " level and actual size ladder projected");
						report.Check(!entry.description.empty() && entry.description.find('{') == std::string::npos, name + " main tokens resolved", entry.description);
						Measure(report, entry.description, 14, ModuleCardText::DetailWidth, 3, name + " full main fits actual font", true);
						Measure(report, entry.description, 13, ModuleCardText::InventoryTextWidth, 1, name + " inventory summary fits", false);
						std::string title = entry.name; for (char& c : title) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
						Measure(report, title, 16, ModuleCardText::DetailWidth, 1, name + " title fits", true, ModuleCardText::TitleFont);
						if (variant == 0 && levelIndex == 3 && !side.IsValid())
						{
							if (definition.moduleId.ToString().find("GlassTopCore") != std::string::npos) report.Check(entry.description == "Attack Power +16,4268; Max Health -6%.", "attribute level and derived percentage visible", entry.description);
							if (definition.moduleId.ToString().find("FrostLock") != std::string::npos) report.Check(entry.description.find("1,6s") != std::string::npos && entry.description.find("6s.") != std::string::npos, "status duration and cooldown L3 visible", entry.description);
							if (definition.moduleId.ToString().find("LuckyCast") != std::string::npos) report.Check(entry.description.find("Luck x 7%") != std::string::npos && entry.description.find("50% cooldown") != std::string::npos, "Luck chance and cooldown fractions visible", entry.description);
						}
						if (side.IsValid())
						{
							const auto* feature = rules->sideFeatureLookup(side);
							const auto expected = IsLargestCellCountVariant(definition, static_cast<int>(variant)) ? ModuleSideFeatureViewState::Active : ModuleSideFeatureViewState::Locked;
							report.Check(feature && entry.sideFeatureState == expected && entry.sideFeatureName == feature->displayName && entry.sideFeatureRequiredCells == ladder.back(), name + " side permission and lock projected");
							report.Check(!entry.sideFeatureDescription.empty() && entry.sideFeatureDescription.find('{') == std::string::npos, name + " side tokens resolved", entry.sideFeatureDescription);
							Measure(report, entry.sideFeatureDescription, 13, ModuleCardText::DetailWidth, 2, name + " full side fits actual font", true);
							if (levelIndex == 3 && expected == ModuleSideFeatureViewState::Active)
							{
								if (side.ToString().find("TrailMines.QuickFuse") != std::string::npos) trailCard = id;
								if (side.ToString().find("GritYourTeeth.CriticalCircuit") != std::string::npos) gritCard = id;
							}
							if (definition.moduleId.ToString().find("RicochetTip") != std::string::npos && levelIndex == 3 && side == definition.sideFeatures.front())
							{
								if (expected == ModuleSideFeatureViewState::Active) ricochetLarge = id; else ricochetSmall = id;
							}
						}
						else report.Check(entry.sideFeatureState == (ladder.size() > 1 ? ModuleSideFeatureViewState::Unselected : ModuleSideFeatureViewState::None), name + " unselected state projected");
					}
		}
		report.Check(modules == 22 && combinations > 200, "all shipped Uncommon combinations covered", { { "modules", modules }, { "combinations", combinations } });
		const auto body = AssetManager::GetAssetManager().LoadFont(ModuleCardText::BodyFont);
		const std::string turkish = u8"\u00c7\u011e\u0130\u00d6\u015e\u00dc\u00e7\u011f\u0131\u00f6\u015f\u00fc";
		for (const auto codepoint : sf::String::fromUtf8(turkish.begin(), turkish.end())) report.Check(body->hasGlyph(codepoint), "body font has Turkish glyph", static_cast<unsigned int>(codepoint));
		Measure(report, turkish + " " + turkish, 14, 80.f, 8, "UTF-8 wraps by codepoint", true);
		report.Check(trailCard && gritCard, "long main and side cards available");
		for (const auto id : { trailCard, gritCard })
		{
			presenter->SetHoverCell(std::nullopt); presenter->PickUpInventory(id); tick();
			int count = 0; CardBounds(report, view, view.GetResolvedRect().position, count); report.Check(count >= 7, "long card actual widgets measured", count);
			Screenshot(report, view, application.GetRenderWindow(), artifacts / (id == trailCard ? "p7-trail.png" : "p7-grit.png")); presenter->CancelHold();
		}
		report.Check(ricochetSmall && ricochetLarge, "locked and active cards available");
		tick(); presenter->PickUpInventory(ricochetSmall); tick();
		report.Check(model->heldInstanceId == ricochetSmall && Entry(*model, ricochetSmall).sideFeatureState == ModuleSideFeatureViewState::Locked, "held small card keeps side locked");
		Screenshot(report, view, application.GetRenderWindow(), artifacts / "p7-locked.png"); presenter->CancelHold();
		presenter->PickUpInventory(ricochetLarge); presenter->RotateHeld(); tick();
		report.Check(model->heldRotation == 1 && Entry(*model, ricochetLarge).sideFeatureState == ModuleSideFeatureViewState::Active, "held rotation retains active side");
		const auto candidate = Place(*rules, player.GetModuleLoadout(), ricochetLarge); report.Check(candidate.has_value(), "large module fits real schematic");
		presenter->RotateHeld(candidate->rotationQuarterTurns - model->heldRotation); presenter->SetHoverCell(candidate->origin); tick();
		report.Check(model->preview.legal && model->preview.cells.size() == 3, "held large card previews real placement");
		Screenshot(report, view, application.GetRenderWindow(), artifacts / "p7-held.png");
		ship->SetInvulnerability(false); ship->ApplyDamage(1.f); report.Check(ship->IsInCombat(), "real damage starts combat");
		report.Check(presenter->DropOnGrid(), "presenter installs through real player"); tick();
		report.Check(Entry(*model, ricochetLarge).rebooting && model->heldInstanceId == 0, "installed card shows combat reboot");
		view.HandleEvent(sf::Event{ sf::Event::MouseMoved{ point(view.CellCenter(candidate->origin)) } }); tick();
		Screenshot(report, view, application.GetRenderWindow(), artifacts / "p7-reboot.png");
		level->TickInternal(3.f); tick(); report.Check(!Entry(*model, ricochetLarge).rebooting && !level->IsPaused(), "reboot finishes while module screen open");
		const auto revision = model->revision.Get(); const auto refresh = view.GetRefreshCount(); const auto loadoutRevision = player.GetModuleLoadout().GetRevision();
		for (int i = 0; i < 10; ++i) tick();
		report.Check(model->revision.Get() == revision && view.GetRefreshCount() == refresh && player.GetModuleLoadout().GetRevision() == loadoutRevision, "idle UI does not mutate loadout or refresh");
		report.Check(player.SetModuleLevel(ricochetLarge, 1), "real player level mutation accepted"); tick();
		report.Check(model->revision.Get() != revision && Entry(*model, ricochetLarge).level == 1 && Entry(*model, ricochetLarge).description.find("25%") != std::string::npos, "level mutation changes card to L1 damage value", Entry(*model, ricochetLarge).description);
		report.Check(player.SetModuleLevel(ricochetLarge, 3), "level restored"); tick();
		report.Check(Entry(*model, ricochetLarge).description.find("35%") != std::string::npos, "L3 card reads actual table", Entry(*model, ricochetLarge).description);
		report.Check(presenter->PickUpAtCell(candidate->origin), "installed module picked up");
		const auto moved = Place(*rules, player.GetModuleLoadout(), ricochetLarge, ricochetLarge, candidate->origin); report.Check(moved.has_value(), "real move target available");
		presenter->RotateHeld(moved->rotationQuarterTurns - candidate->rotationQuarterTurns); presenter->SetHoverCell(moved->origin);
		report.Check(presenter->DropOnGrid(), "module moved through presenter"); tick();
		report.Check(!Entry(*model, ricochetLarge).rebooting && Entry(*model, ricochetLarge).level == 3, "move preserves level and does not reboot");
		view.HandleEvent(sf::Event{ sf::Event::MouseMoved{ point(view.CellCenter(moved->origin)) } }); tick();
		int count = 0; CardBounds(report, view, view.GetResolvedRect().position, count); report.Check(count >= 7, "actual card widgets measured", count);
		Screenshot(report, view, application.GetRenderWindow(), artifacts / "p7-installed.png");
		report.Check(presenter->PickUpAtCell(moved->origin) && presenter->DropOnInventory(), "module uninstalled through presenter"); tick();
		const auto sideRevision = model->revision.Get(); const auto selectedSide = player.GetModuleLoadout().FindInventoryItem(ricochetLarge)->sideFeatureId;
		report.Check(player.SetModuleSideFeature(ricochetLarge, "Module.SideFeature.RicochetTip.SecondBounce"), "real player side selection changed"); tick();
		report.Check(model->revision.Get() != sideRevision && Entry(*model, ricochetLarge).sideFeatureDescription.find("15%") != std::string::npos, "selected side change refreshes card description", Entry(*model, ricochetLarge).sideFeatureDescription);
		report.Check(player.SetModuleSideFeature(ricochetLarge, selectedSide), "side selection restored"); tick();
		const auto handles = ship->GetModuleApplier().GetOwnedHandleCount();
		for (int i = 0; i < 10; ++i)
		{
			const auto placement = Place(*rules, player.GetModuleLoadout(), ricochetLarge); report.Check(placement.has_value(), "cycle placement available");
			presenter->PickUpInventory(ricochetLarge); presenter->RotateHeld(placement->rotationQuarterTurns); presenter->SetHoverCell(placement->origin);
			report.Check(presenter->DropOnGrid(), "cycle installed"); tick();
			report.Check(presenter->PickUpAtCell(placement->origin) && presenter->DropOnInventory(), "cycle uninstalled"); tick();
			report.Check(ship->GetModuleApplier().GetOwnedHandleCount() == handles && ship->GetModuleApplier().GetGeneratedActorCount() == 0 && Entry(*model, ricochetLarge).sideFeatureState == ModuleSideFeatureViewState::Active, "cycle cleans runtime and preserves card");
		}
		int row = 0; for (std::size_t i = 0; i < model->inventory.size(); ++i) if (model->inventory[i].instanceId == ricochetLarge) row = static_cast<int>(i);
		while (view.GetInventoryScroll() + ModuleScreenView::InventoryVisibleRows <= row) view.HandleEvent(sf::Event{ sf::Event::MouseWheelScrolled{ sf::Mouse::Wheel::Vertical, -1.f, point(view.InventoryRowCenter(0)) } });
		tick(); row -= view.GetInventoryScroll();
		report.Check(view.InventoryItemAt(view.InventoryRowCenter(row)) == ricochetLarge, "real inventory scroll finds returned module");
		view.HandleEvent(sf::Event{ sf::Event::MouseMoved{ point(view.InventoryRowCenter(row)) } }); tick();
		Screenshot(report, view, application.GetRenderWindow(), artifacts / "p7-cargo.png");
		presenter->SetOpen(false); tick(); report.Check(!view.GetVisibility(), "close hides screen"); presenter->SetOpen(true); tick();
		report.Check(view.GetVisibility(), "reopen clears stale hover and displays screen");
		// Load real content through its public loader with a bad numeric key, then without an optional description.
		const auto source = std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/module_side_features.json";
		std::ifstream input{ source }; Json fixture; input >> fixture; fixture["sideFeatures"][0]["description"] = "Value {missing.numeric}";
		const auto fixturePath = artifacts / "p7-side-fixture.json"; common_module_e2e::WriteArtifact(fixturePath, fixture);
		const auto rejected = content::ModuleSideFeatureLoader::LoadFromFile(fixturePath);
		report.Check(!rejected.Succeeded() && rejected.error.find("missing.numeric") != std::string::npos, "public loader rejects unknown description token", rejected.error);
		fixture["sideFeatures"][0].erase("description"); common_module_e2e::WriteArtifact(fixturePath, fixture);
		report.Check(content::ModuleSideFeatureLoader::LoadFromFile(fixturePath).Succeeded(), "legacy content without optional side description accepted");
	}
}

int main(int argc, char** argv)
{
	if (argc != 2) return 2;
	Report report; const std::filesystem::path artifact{ argv[1] }; std::filesystem::create_directories(artifact.parent_path());
	try
	{
		AssetManager::GetAssetManager().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "P7 content registration failed" };
		std::string title = "Uncommon P7 E2E"; Application application{ sf::Vector2u{ 1920u, 1080u }, 32u, title, sf::Style::None }; application.GetRenderWindow().setVisible(false);
		Run(report, application, artifact.parent_path()); report.artifact["passed"] = true;
	}
	catch (const std::exception& error) { report.artifact["error"] = error.what(); std::cerr << error.what() << '\n'; }
	report.artifact["test"] = "UncommonModuleP7E2E"; common_module_e2e::WriteArtifact(artifact, report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
