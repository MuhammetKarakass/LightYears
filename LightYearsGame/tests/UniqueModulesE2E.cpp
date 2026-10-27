#include "framework/EngineContext.h"
#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP6E2E.h"
#include "presentation/hud/modules/HoloDraw.h"
namespace
{
	using namespace uncommon_p6_e2e;
	constexpr const char* Brace = "Module.Defense.BraceReflex.Basic";
	constexpr const char* Feedback = "Module.Offense.EnergyFeedback.Basic";
	struct Rig
	{
		Report& report;
		ShipSchematic schematic;
		List<ModuleDefinition> supplements;
		ModuleRulesContext rules;
		ModuleLoadout loadout;
		shared_ptr<HeadlessModuleLevel> level;
		shared_ptr<PlayerSpaceShip> ship;
		Rig(Report& result, Application& application) : report{ result }, schematic{ *content::ShipContentCatalog::GetPlayerFighterDefinition().schematic }, rules{ schematic, [this](const sas::ContentId& id) { for (const auto& module : supplements) if (module.moduleId == id) return &module; return content::ModuleContentCatalog::FindById(id); }, content::ModuleContentCatalog::GetZoneTable(), [](const sas::ContentId& id) { return content::ModuleSideFeatureCatalog::FindById(id); } }, level{ std::make_shared<HeadlessModuleLevel>(&application) }
		{
			// A test annex isolates geometry; defaults, rules, abilities and the ship remain shipped content.
			schematic.height += 8; schematic.cellZones.resize(schematic.width * schematic.height, GameplayTags::Module::Zone::Hull); schematic.cellLocked.resize(schematic.width * schematic.height, 0);
			level->BeginPlayInternal(); ship = level->SpawnActor<PlayerSpaceShip>(content::ShipContentCatalog::GetPlayerFighterDefinition()).lock();
			report.Check(ship != nullptr, "spawn real player ship"); level->TickInternal(0.f);
			ship->SetInvulnerability(false); ship->SetUseScreenClamp(false); ship->SetActorLocation({ 10000.f, 10000.f });
			report.Check(loadout.SeedDefaults(rules) && ship->GetModuleApplier().Apply(loadout, rules), "real default loadout and applier");
		}
		~Rig() { if (ship) ship->Destroy(); ship.reset(); level.reset(); }
		LightYearsAbilitySystemComponent& Asc() { return ship->GetAbilitySystemComponent(); }
		ModuleTriggerService& Triggers() { return Asc().GetModuleTriggerService(); }
		void Apply() { report.Check(ship->GetModuleApplier().Apply(loadout, rules), "apply changed loadout"); }
		std::uint32_t Add(const char* id, sf::Vector2i origin, int rotation = 0, int variant = 0, const char* side = "")
		{
			const auto* definition = rules.lookup(id); report.Check(definition != nullptr, std::string{ "load content " } + id);
			const auto instance = loadout.AddToInventory(rules, *definition, variant, definition->rarity, 1, side); report.Check(instance != 0, "validated inventory instance");
			const auto failure = loadout.Install(rules, instance, origin, rotation);
			report.Check(failure == ModulePlacementFailure::None, std::string{ "legal installation " } + id, ModulePlacementRules::Describe(failure)); Apply(); return instance;
		}
		void Remove(std::uint32_t id) { report.Check(loadout.Uninstall(id), "uninstall"); Apply(); }
		void Move(std::uint32_t id, sf::Vector2i origin, int rotation = 0) { report.Check(loadout.Move(rules, id, origin, rotation) == ModulePlacementFailure::None, "move or rotate"); Apply(); }
		void Tick(float seconds) { ship->GetModuleTriggerRuntime().Tick(seconds); ship->GetModuleApplier().Tick(seconds); }
		GameAbility& Ability(const char* id, sas::AbilitySlot slot)
		{
			if (auto* old = Asc().GetAbilityById(id); old && old->GetDefinition().slot == slot) return *old;
			if (auto* old = Asc().GetAbility(slot)) report.Check(Asc().RemoveAbility(old->GetHandle()), "replace test slot");
			if (auto* old = Asc().GetAbilityById(id)) report.Check(Asc().RemoveAbility(old->GetHandle()), "move test ability");
			auto definition = *content::AbilityContentCatalog::FindById(id); definition.slot = slot;
			report.Check(Asc().GrantAbility(definition).IsValid(), "grant shipped ability"); return *Asc().GetAbility(slot);
		}
		void Use(const char* id, sas::AbilitySlot slot)
		{
			auto& ability = Ability(id, slot); if (ability.IsActive()) ability.Cancel(sas::AbilityEndReason::Completed); ability.Tick(1000.f);
			report.Check(ability.TryActivate(), std::string{ "real successful activation " } + id); if (ability.IsActive()) ability.Cancel(sas::AbilityEndReason::Completed);
		}
		void Defense() { Use("Ability.Defense.Shield.Basic", sas::AbilitySlot::Ability1); }
		void Offense() { Use("Ability.Offense.Rocket.Basic", sas::AbilitySlot::Ability3); }
		float Armor() { return Asc().GetAttributes().GetCurrentValue(OwnerAttributeIds::Armor); }
		void Equal(const char* label, float expected, float actual) { report.CheckValue(label, expected, actual, "real ship / loadout / applier"); }
		void DuplicateKind(const char* source, const char* id) { auto copy = *rules.lookup(source); copy.moduleId = id; supplements.push_back(std::move(copy)); }
	};
	void ContentAndGeometry(Report& report, Application& application)
	{
		Rig f{ report, application }; int count = 0;
		for (const auto& module : content::ModuleContentCatalog::GetDefinitions())
		{
			if (!module.unique) continue; ++count;
			report.Check(module.rarity == ModuleRarity::None && !module.family.IsValid() && !module.systemCategory.IsValid() && module.levelCount == 1 && module.sideFeatures.empty() && module.sharedPools.empty() && module.sizeVariants.size() == 1 && module.sizeVariants.front().shape.cells == List<sf::Vector2i>{ { 0, 0 } }, "one-cell Unique class without rarity, family or progression");
			ModulePlacement candidate; candidate.moduleId = module.moduleId; candidate.origin = { 5, 5 };
			report.Check(ModulePlacementRules::CanPlace(f.rules, List<ModulePlacement>{}, candidate) == ModulePlacementFailure::SystemZoneMismatch, "Unique excluded from AbilityBay");
			report.Check(ModuleResolver::GetZoneLevelBonus(f.rules, module, candidate) == 0, "Unique has no zone bonus");
			const auto inventory = f.loadout.AddToInventory(f.rules, module, 0, ModuleRarity::Epic, 1);
			report.Check(f.loadout.FindInventoryItem(inventory)->rarity == ModuleRarity::None && !f.loadout.SetLevel(f.rules, inventory, 2) && !f.loadout.SetSideFeature(f.rules, inventory, "Module.SideFeature.Shared.D1.Rare"), "inventory rejects Unique progression");
		}
		report.Check(count == 7, "seven shipped Uniques");
		f.Ability("Ability.Defense.Shield.Basic", sas::AbilitySlot::Ability1); f.Ability("Ability.Utility.VoidGate.Basic", sas::AbilitySlot::Ability2); f.Ability("Ability.Movement.Dash.Basic", sas::AbilitySlot::Ability4);
		auto& offense = f.Ability("Ability.Offense.Rocket.Basic", sas::AbilitySlot::Ability3);
		report.Check(f.Asc().HasDistinctLoadoutFamilies(offense.GetHandle()), "Four Corners sees four actual equipped ability families");
		f.Add("Module.Utility.ConfluenceCore.Epic", { 1, 11 }); const auto before = f.ship->GetModuleApplier().GetResolvedEffects(); int x = 3;
		for (const auto* name : { "Amplifier", "Relay", "Echo", "Mirror", "Pulsar", "Spark", "ProfitShare" }) { const auto id = std::string{ "Module.Unique." } + name; f.Add(id.c_str(), { x++, 16 }); }
		const auto after = f.ship->GetModuleApplier().GetResolvedEffects();
		report.Check(f.Asc().HasDistinctLoadoutFamilies(offense.GetHandle()), "seven Uniques never contribute to or invalidate Four Corners families");
		for (const auto& placement : f.loadout.GetPlacements()) if (f.rules.lookup(placement.moduleId)->unique) report.Check(ModuleResolver::FindLevelBonus(after, placement.instanceId) == 0, "Amplifier never levels another Unique");
		ModulePlacement duplicate; duplicate.moduleId = "Module.Unique.Amplifier"; duplicate.origin = { 12, 16 };
		report.Check(ModulePlacementRules::CanPlace(f.rules, f.loadout, duplicate) == ModulePlacementFailure::DuplicateModule, "one installed instance per Unique id");
		report.Check(before.ownerModifiers.size() == after.ownerModifiers.size() && before.abilityRules.size() == after.abilityRules.size() && before.primaryWeaponModifiers.size() == after.primaryWeaponModifiers.size() && before.distinctFamilyMainEffectBonus == after.distinctFamilyMainEffectBonus, "Uniques add no Confluence family count or Four Corners side rule");
		for (std::size_t i = 0; i < before.ownerModifiers.size(); ++i) f.Equal("Confluence unchanged", before.ownerModifiers[i].magnitude, after.ownerModifiers[i].magnitude);
		ModulePlacement unique; unique.moduleId = "Module.Unique.Mirror"; unique.origin = { 4, 12 };
		ModulePlacement target; target.moduleId = Brace; target.origin = { 5, 11 }; target.rotationQuarterTurns = 1;
		report.Check(ModulePlacementRules::SharesCellEdge(*f.rules.lookup(unique.moduleId), unique, *f.rules.lookup(Brace), target), "rotated domino edge");
		target.rotationQuarterTurns = 0; report.Check(!ModulePlacementRules::SharesCellEdge(*f.rules.lookup(unique.moduleId), unique, *f.rules.lookup(Brace), target), "diagonal negative");
		target.fragmented = true; target.fragmentCells = { { 3, 12 }, { 8, 14 } }; report.Check(ModulePlacementRules::SharesCellEdge(*f.rules.lookup(unique.moduleId), unique, *f.rules.lookup(Brace), target), "fragment cells determine adjacency");
		for (int rotation = 0; rotation < 4; ++rotation)
		{
			const auto rotated = ModuleShapeRules::Rotate({ { 1, 1 }, { 1, 0 } }, rotation); report.Check(ModulePlacementRules::FacingOffset(rotation) == rotated[1] - rotated[0], "Mirror facing follows clockwise rotation");
			sf::VertexArray arrow{ sf::PrimitiveType::Triangles }; holo::FacingArrow(arrow, {}, rotation, 10.f, sf::Color::White); report.Check(arrow.getVertexCount() == 18, "visible Mirror arrow geometry");
		}
		std::ifstream input{ std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/modules.json" }; Json source; input >> source;
		auto& authored = *std::find_if(source["modules"].begin(), source["modules"].end(), [](const Json& entry) { return entry.at("id") == "Module.Unique.Echo"; }); const auto original = authored;
		for (const auto* field : { "rarity", "family", "levelCount", "sideFeatures", "sharedPools", "sizeLadder", "systemCategory" }) { authored = original; authored[field] = 1; report.Check(!content::ModuleLoader::LoadFromJson(source, "Module.Unique.Echo").Succeeded(), std::string{ "loader rejects " } + field); }
		for (const auto bad : { 0.f, -1.f, 1.5f }) { authored = original; authored["unique"]["echoPowerFraction"] = bad; report.Check(!content::ModuleLoader::LoadFromJson(source, "Module.Unique.Echo").Succeeded(), "loader rejects invalid echo power"); }
		authored = original; authored["sizeVariants"][0]["triggerBuffs"] = Json::array(); report.Check(!content::ModuleLoader::LoadFromJson(source, "Module.Unique.Echo").Succeeded(), "Unique cannot smuggle ordinary rules");
	}
	void Amplifier(Report& report, Application& application)
	{
		Rig f{ report, application }; const float baseline = f.Asc().GetAttributes().GetCurrentValue(OwnerAttributeIds::AttackPower);
		const auto assault = f.Add("Module.Offense.AssaultCore.Basic", { 5, 12 }); f.Equal("Hull zone +2", 19.f, f.Asc().GetAttributes().GetCurrentValue(OwnerAttributeIds::AttackPower) - baseline);
		const auto amp = f.Add("Module.Unique.Amplifier", { 4, 12 }); f.Equal("Amplifier on top of zone +2", 21.f, f.Asc().GetAttributes().GetCurrentValue(OwnerAttributeIds::AttackPower) - baseline);
		f.Equal("zone remains separately capped", 2.f, static_cast<float>(ModuleResolver::FindZoneLevelBonus(f.ship->GetModuleApplier().GetResolvedEffects(), assault)));
		f.DuplicateKind("Module.Unique.Amplifier", "Module.Unique.AmplifierCopy"); const auto copy = f.Add("Module.Unique.AmplifierCopy", { 5, 11 });
		f.Equal("same-kind Amplifiers do not stack", 21.f, f.Asc().GetAttributes().GetCurrentValue(OwnerAttributeIds::AttackPower) - baseline);
		f.Remove(amp); f.Move(copy, { 1, 13 }); f.Equal("uninstall removes bonus", 19.f, f.Asc().GetAttributes().GetCurrentValue(OwnerAttributeIds::AttackPower) - baseline);
		auto& ability = f.Ability("Ability.Offense.Rocket.Basic", sas::AbilitySlot::Ability1); const auto stored = ability.GetLevel();
		const auto shellAmp = f.Add("Module.Unique.Amplifier", { 5, 6 }); f.Equal("Q temporary level bonus", 1.f, static_cast<float>(f.Asc().GetScopedAbilityLevelBonus(ability.GetDefinition()))); report.Check(ability.GetLevel() == stored, "earned level unchanged");
		f.Remove(shellAmp); f.Equal("Q bonus removed", 0.f, static_cast<float>(f.Asc().GetScopedAbilityLevelBonus(ability.GetDefinition())));
	}
	void RelayEchoProfit(Report& report, Application& application)
	{
		{
			Rig f{ report, application }; const auto relay = f.Add("Module.Unique.Relay", { 5, 12 }); const auto brace = f.Add(Brace, { 6, 12 }); const auto feedback = f.Add(Feedback, { 4, 12 }); f.Add("Module.Unique.Echo", { 3, 12 });
			f.DuplicateKind("Module.Unique.Relay", "Module.Unique.RelayCopy"); f.Add("Module.Unique.RelayCopy", { 5, 13 });
			auto wallet = std::make_shared<RunGold>(); f.ship->GetModuleApplier().SetGoldWallet(wallet); f.Add("Module.Unique.ProfitShare", { 4, 11 });
			const auto uses = f.Asc().GetLoadoutAbilityActivationCount(); const float energy = f.Asc().GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower); f.Defense();
			report.Check(f.Triggers().GetNaturalCount(brace) == 1 && f.Triggers().GetForcedCount(feedback) == 1, "Relay forces other touching module once at full power");
			f.Equal("forced feedback main magnitude", energy + 20.f, f.Asc().GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower)); f.Tick(1.f);
			report.Check(f.Triggers().GetForcedCount(feedback) == 1 && wallet->GetAmount() == 0 && f.Asc().GetLoadoutAbilityActivationCount() == uses + 1, "forced target never feeds Echo, Gold or ability activations");
			report.Check(!f.Triggers().Force(relay), "Uniques never affect other Uniques");
		}
		{
			Rig f{ report, application }; const auto brace = f.Add(Brace, { 5, 12 }); f.Add("Module.Unique.Echo", { 4, 12 }); f.DuplicateKind("Module.Unique.Echo", "Module.Unique.EchoCopy"); f.Add("Module.Unique.EchoCopy", { 5, 11 }); f.Add("Module.Unique.ProfitShare", { 6, 11 });
			auto wallet = std::make_shared<RunGold>(); f.ship->GetModuleApplier().SetGoldWallet(wallet); const float armor = f.Armor();
			f.Defense(); f.Equal("natural Brace magnitude", armor + 31.f, f.Armor()); report.Check(wallet->GetAmount() == 1, "natural trigger grants Gold");
			f.Defense(); report.Check(wallet->GetAmount() == 1, "one-second Gold cap"); f.Tick(.99f); report.Check(f.Triggers().GetForcedCount(brace) == 0, "no early Echo");
			f.Tick(.01001f); f.Equal("Echo half magnitude", armor + 15.5f, f.Armor()); report.Check(f.Triggers().GetForcedCount(brace) == 2 && wallet->GetAmount() == 1, "one Echo per natural trigger with same-kind no-stack and no Gold");
			f.Add("Module.Unique.Mirror", { 1, 11 }); f.Equal("unrelated reapply preserves Echo power", armor + 15.5f, f.Armor());
			f.Tick(2.f); report.Check(f.Triggers().GetForcedCount(brace) == 2, "Echo never chains"); f.Defense(); report.Check(wallet->GetAmount() == 2, "Gold cap expires");
		}
		{
			Rig f{ report, application }; f.Add("Module.Unique.Relay", { 5, 12 }); f.Add(Brace, { 4, 12 }, 1);
			const auto slam = f.Add("Module.Defense.TemperedHull.Rare", { 6, 12 }, 0, 1, "Module.SideFeature.TemperedHull.BastionSlam"); f.Defense(); const auto naturally = f.Triggers().GetNaturalCount(slam);
			report.Check(!f.Triggers().Force(slam), "Bastion cooldown blocks forced trigger"); f.Tick(4.99f); report.Check(!f.Triggers().Force(slam), "failed force never resets lockout");
			f.Tick(.02f); report.Check(f.Triggers().Force(slam), "Bastion fires at original cooldown expiry"); report.Check(f.Triggers().GetNaturalCount(slam) == naturally, "forced owner burst is never natural");
		}
	}
	void MirrorPulsarSpark(Report& report, Application& application)
	{
		{
			Rig f{ report, application }; const auto mirror = f.Add("Module.Unique.Mirror", { 5, 13 }); const auto brace = f.Add(Brace, { 5, 12 });
			f.Offense(); report.Check(f.Triggers().GetNaturalCount(brace) == 1, "Mirror Defense rule fires naturally on Offense in front");
			f.Move(mirror, { 5, 13 }, 1); f.Offense(); report.Check(f.Triggers().GetNaturalCount(brace) == 1, "Mirror facing rotates away");
			f.Move(brace, { 6, 13 }, 1); f.Offense(); report.Check(f.Triggers().GetNaturalCount(brace) == 2, "Mirror rotated front follows target");
			f.Move(mirror, { 4, 12 }, 1); f.Offense(); report.Check(f.Triggers().GetNaturalCount(brace) == 2, "Mirror diagonal negative");
		}
		{
			Rig f{ report, application }; f.Add("Module.Unique.Pulsar", { 4, 12 }); const auto brace = f.Add(Brace, { 5, 12 }); f.Tick(20.f); report.Check(f.Triggers().GetForcedCount(brace) == 0, "Pulsar pauses outside combat");
			const auto enemy = f.level->SpawnActor<SpaceShip>(content::ShipContentCatalog::GetPlayerFighterDefinition()).lock(); f.level->TickInternal(0.f); enemy->SetActorLocation({ 20000.f, 20000.f });
			ApplyCombatDamage(*enemy, 1.f, f.ship.get(), {}, Plain()); report.Check(f.ship->IsInCombat(), "real damage enters combat");
			f.Tick(11.99f); report.Check(f.Triggers().GetForcedCount(brace) == 0, "Pulsar not before 12s"); f.Tick(.01f); report.Check(f.Triggers().GetForcedCount(brace) == 1, "Pulsar at 12s");
		}
		{
			Rig f{ report, application }; f.Add("Module.Unique.Spark", { 5, 6 }); const auto feedback = f.Add(Feedback, { 4, 6 }); const auto uses = f.Asc().GetLoadoutAbilityActivationCount();
			f.Use("Ability.Offense.Rocket.Basic", sas::AbilitySlot::Ability1); report.Check(f.Triggers().GetForcedCount(feedback) == 1 && f.Asc().GetLoadoutAbilityActivationCount() == uses + 1, "Spark reacts to touching Q success without activating abilities");
			f.Use("Ability.Offense.IonStorm.Basic", sas::AbilitySlot::Ability3); report.Check(f.Triggers().GetForcedCount(feedback) == 1, "Spark ignores non-touching shell");
			std::uint32_t shellId = 0;
			for (const auto& placement : f.loadout.GetPlacements()) if (f.rules.lookup(placement.moduleId)->systemCategory == GameplayTags::Module::System::AbilitySlot1) shellId = placement.instanceId;
			f.Remove(shellId);
			const auto ultimate = f.Add("Module.AbilityModule.UltimateProtocol", { 5, 5 });
			f.Use("Ability.Offense.Rocket.Basic", sas::AbilitySlot::Ability1);
			report.Check(f.Triggers().GetForcedCount(feedback) == 2, "Spark recognizes touching Ultimate Protocol as Q shell");
			f.Remove(ultimate);
			auto* q = f.Asc().GetAbility(sas::AbilitySlot::Ability1); report.Check(q && !q->TryActivate() && f.Triggers().GetForcedCount(feedback) == 2, "failed activation never feeds Spark");
			const auto continuous = f.Add("Module.Offense.AssaultCore.Basic", { 5, 12 }); report.Check(!f.Triggers().Force(continuous), "continuous placements cannot be force-triggered");
		}
	}
	void Schematics(Report& report)
	{
		for (const auto& id : content::ShipContentCatalog::GetPlayerShipIds())
		{
			const auto& schema = *content::ShipContentCatalog::GetPlayerShipDefinition(id).schematic;
			for (const auto& shell : schema.defaultModules)
			{
				const auto* definition = content::ModuleContentCatalog::FindById(shell.moduleId); const auto slot = AbilitySlotForModuleSystem(definition->systemCategory); if (slot == sas::AbilitySlot::None) continue;
				int normal = 0;
				for (int direction = 0; direction < 4; ++direction) { const auto cell = shell.origin + ModulePlacementRules::FacingOffset(direction); const auto& zone = schema.GetZoneAt(cell.x, cell.y); if (zone.IsValid() && zone != GameplayTags::Module::Zone::AbilityBay && !schema.IsLockedAt(cell.x, cell.y)) ++normal; }
				report.artifact["schematicAdjacency"].push_back({ { "ship", id }, { "slot", static_cast<int>(slot) }, { "normalUnlockedNeighbours", normal } });
				report.Check(normal > 0, "normal edge neighbour for shipped shell: " + id);
			}
		}
	}
}
int main(int argc, char** argv)
{
	vega::EngineContext context; vega::EngineContext::Scope scope{ context }; vega::InstallVegaEngineAssets(); if (argc != 2) return 2;
	Report report; report.artifact["test"] = "UniqueModulesE2E";
	try
	{
		context.Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Unique content registration failed" };
		std::string title = "Unique Modules E2E"; Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None }; application.GetRenderWindow().setVisible(false);
		Schematics(report); ContentAndGeometry(report, application); Amplifier(report, application); RelayEchoProfit(report, application); MirrorPulsarSpark(report, application); report.artifact["passed"] = true;
	}
	catch (const std::exception& error) { report.artifact["error"] = error.what(); std::cerr << error.what() << '\n'; }
	WriteArtifact(argv[1], report.artifact); return report.artifact.value("passed", false) ? 0 : 1;
}
