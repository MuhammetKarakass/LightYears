#include "framework/EngineContext.h"
#include "attributes/AttributeMath.h"
#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP6E2E.h"
#include "gameplay/ability/rocket/RocketProjectileActor.h"
#include "gameplay/ability/directionalBarrier/DirectionalBarrierContracts.h"
#include "gameplay/ability/voidGate/VoidGatePortalActor.h"
#include "player/Player.h"
#include "player/PlayerManager.h"

namespace
{
	using namespace uncommon_p6_e2e;
	constexpr const char* UltimateId = "Module.AbilityModule.UltimateProtocol";
	constexpr const char* RocketId = "Ability.Offense.Rocket.Basic";

	GameAbility& Grant(Fixture& f, const char* id, sas::AbilitySlot slot)
	{
		auto& asc = f.ship->GetAbilitySystemComponent();
		if (const auto* old = asc.GetAbility(slot)) f.report.Check(asc.RemoveAbility(old->GetHandle()), "replace only the selected ability slot");
		if (const auto* previous = asc.GetAbilityById(id)) f.report.Check(asc.RemoveAbility(previous->GetHandle()), "move an existing ability without duplicating its content ID");
		const auto* authored = content::AbilityContentCatalog::FindById(id);
		f.report.Check(authored != nullptr, std::string{ "shipped ability " } + id);
		auto definition = *authored; definition.slot = slot;
		f.report.Check(asc.GrantAbility(definition).IsValid(), "grant shipped content through ASC");
		return *asc.GetAbility(slot);
	}

	void Start(Fixture& f, GameAbility& ability)
	{
		if (ability.IsActive()) ability.Cancel(sas::AbilityEndReason::Completed);
		ability.Tick(1000.f);
		f.report.Check(ability.TryActivate(), "activate through the real module-gated ASC");
	}

	ModulePlacement Standard(Fixture& f, sas::AbilitySlot slot)
	{
		for (const auto& placement : f.loadout.GetPlacements())
			if (AbilitySlotForModuleSystem(f.rules.lookup(placement.moduleId)->systemCategory) == slot) return placement;
		throw std::runtime_error{ "missing standard slot shell" };
	}

	std::uint32_t Replace(Fixture& f, sas::AbilitySlot slot)
	{
		const auto shell = Standard(f, slot);
		f.Remove(shell.instanceId);
		const auto& definition = *f.rules.lookup(UltimateId);
		const auto instance = f.loadout.AddToInventory(f.rules, definition, 0, definition.rarity, 1);
		f.report.Check(instance != 0 && f.loadout.Install(f.rules, instance, shell.origin, 0) == ModulePlacementFailure::None, "replace standard shell with Ultimate Protocol");
		f.report.Check(f.loadout.FindPlacement(instance)->abilitySlot == slot, "installed instance captures its selected slot");
		f.report.Check(f.ship->GetModuleApplier().Apply(f.loadout, f.rules), "apply replacement through ship module runtime");
		f.report.Check(f.ship->GetAbilitySystemComponent().IsModuleSystemEnabled(ModuleSystemForAbilitySlot(slot)), "replacement supplies the standard slot system");
		f.Equal("ability module receives no zone levels", 0.f, static_cast<float>(ModuleResolver::FindZoneLevelBonus(f.ship->GetModuleApplier().GetResolvedEffects(), instance)));
		return instance;
	}

	shared_ptr<RocketProjectileActor> Fire(Fixture& f, GameAbility& rocket)
	{
		for (const auto& actor : Actors<RocketProjectileActor>(f)) actor->Destroy();
		Start(f, rocket);
		const auto actors = Actors<RocketProjectileActor>(f);
		f.report.Check(actors.size() == 1, "Rocket activation spawns one real projectile");
		return actors.front();
	}

	float Deliver(Fixture& f, const shared_ptr<RocketProjectileActor>& projectile)
	{
		const auto enemy = Enemy(f, 600.f);
		enemy->SetCollisionMask(vega::GameCollision::PlayerBullet);
		projectile->SetDamagePayload(Plain());
		projectile->SetActorLocation(enemy->GetActorLocation());
		f.level->TickInternal(0.f);
		const float before = enemy->GetHealthComponent().GetHealth();
		projectile->OnActorBeginOverlap(enemy.get());
		const float damage = before - enemy->GetHealthComponent().GetHealth();
		f.report.Check(damage > 0.f && projectile->GetIsPendingDestroy(), "real Rocket explosion damages the target and consumes the projectile", { { "damage", damage }, { "consumed", projectile->GetIsPendingDestroy() } });
		enemy->Destroy();
		return damage;
	}

	void RocketAndRemoval(Report& report, Application& application)
	{
		Fixture f{ report, application };
		auto& rocket = Grant(f, RocketId, sas::AbilitySlot::Ability4);
		const auto shell = Standard(f, sas::AbilitySlot::Ability4);
		const float cooldown = rocket.GetCooldownDuration();
		const auto baseline = Fire(f, rocket);
		const float baseDamage = baseline->GetDamage();
		const float delivered = Deliver(f, baseline);
		const auto instance = Replace(f, sas::AbilitySlot::Ability4);
		const auto powered = Fire(f, rocket);
		f.Equal("Ultimate Rocket captures main effect x1.6", 1.6f, rocket.GetDefinition().mainEffectMultiplier);
		f.Equal("Rocket projectile damage x1.6", baseDamage * 1.6f, powered->GetDamage());
		f.Equal("Rocket cooldown x2", cooldown * 2.f, rocket.GetCooldownDuration());
		f.Equal("Rocket starts an actual doubled cooldown", cooldown * 2.f, rocket.GetCooldownRemaining());
		f.Remove(instance);
		report.Check(!f.ship->GetAbilitySystemComponent().IsModuleAbilityAvailable(RocketId), "removing Ultimate disables R without its standard shell");
		f.Equal("launched Rocket keeps its captured damage after removal", delivered * 1.6f, Deliver(f, powered));
		f.Equal("running cooldown keeps its captured x2 after removal", cooldown * 2.f, rocket.GetCooldownDuration());
		rocket.Tick(1000.f);
		report.Check(!rocket.TryActivate(), "missing R shell blocks a ready Rocket");
		report.Check(f.loadout.Install(f.rules, shell.instanceId, shell.origin, 0) == ModulePlacementFailure::None && f.ship->GetModuleApplier().Apply(f.loadout, f.rules), "reinstall standard R shell");
		const auto normal = Fire(f, rocket);
		f.Equal("standard shell restores normal Rocket damage", baseDamage, normal->GetDamage());
		f.Equal("standard shell restores normal cooldown", cooldown, rocket.GetCooldownDuration());
	}

	void SlotsAndCounts(Report& report, Application& application)
	{
		Fixture f{ report, application };
		auto& q = Grant(f, RocketId, sas::AbilitySlot::Ability1);
		auto& r = *f.ship->GetAbilitySystemComponent().GetAbility(sas::AbilitySlot::Ability4);
		const float qCooldown = q.GetCooldownDuration(), rCooldown = r.GetCooldownDuration();
		const auto qModule = Replace(f, sas::AbilitySlot::Ability1);
		f.Equal("Q Ultimate affects Q cooldown", qCooldown * 2.f, q.GetCooldownDuration());
		f.Equal("Q Ultimate leaves R cooldown unchanged", rCooldown, r.GetCooldownDuration());
		f.Equal("Q Ultimate leaves R main effect unchanged", 1.f, f.ship->GetAbilitySystemComponent().GetAbilityMainEffectMultiplier(r.GetDefinition()));
		const auto rModule = Replace(f, sas::AbilitySlot::Ability4);
		f.Equal("two Ultimate modules affect R independently", rCooldown * 2.f, r.GetCooldownDuration());
		f.Equal("two Ultimate modules never double stack on Q", qCooldown * 2.f, q.GetCooldownDuration());
		Start(f, q);
		f.Equal("Q ability captures its own module", 1.6f, q.GetDefinition().mainEffectMultiplier);
		for (const auto slot : { sas::AbilitySlot::Ability2, sas::AbilitySlot::Ability3 }) Replace(f, slot);
		report.Check(f.ship->GetModuleApplier().GetResolvedEffects().abilityModules.size() == 4, "four copies may fill all four slots");
		const auto& module = *f.rules.lookup(UltimateId);
		const auto extra = f.loadout.AddToInventory(f.rules, module, 0, module.rarity, 1);
		ModulePlacement duplicate = *f.loadout.FindInventoryItem(extra);
		duplicate.abilitySlot = sas::AbilitySlot::Ability1;
		duplicate.origin = { 0, 0 };
		// Isolate logical exclusivity from geometric overlap with an additional valid bay cell.
		auto schematic = f.rules.schematic;
		schematic.cellZones[0] = GameplayTags::Module::Zone::AbilityBay; schematic.cellLocked[0] = 0;
		ModuleRulesContext rules{ schematic, f.rules.lookup, f.rules.zoneTable, f.rules.sideFeatureLookup };
		report.Check(ModulePlacementRules::CanPlace(rules, f.loadout, duplicate) == ModulePlacementFailure::DuplicateSystem, "second Ultimate for the same slot is rejected away from its cell");
		ModulePlacement standard; standard.moduleId = "Module.Utility.AbilitySlot1.Standard"; standard.origin = { 0, 0 };
		report.Check(ModulePlacementRules::CanPlace(rules, f.loadout, standard) == ModulePlacementFailure::DuplicateSystem, "standard plus Ultimate for the same slot is rejected away from its cell");
		report.Check(!f.loadout.SetLevel(f.rules, qModule, 2), "ability modules cannot gain levels");
		report.Check(!f.loadout.SetSideFeature(f.rules, qModule, "Module.SideFeature.FieldMastery.KnowledgeTransfer"), "ability modules cannot gain side features");
		auto restored = f.loadout;
		const auto restoredShip = f.level->SpawnActor<PlayerSpaceShip>(content::ShipContentCatalog::GetPlayerFighterDefinition()).lock();
		report.Check(restoredShip && restored.FindPlacement(rModule)->abilitySlot == sas::AbilitySlot::Ability4 && restoredShip->GetModuleApplier().Apply(restored, f.rules), "loadout restoration preserves instance slot bindings");
		report.Check(restoredShip->GetAbilitySystemComponent().IsModuleSystemEnabled(GameplayTags::Module::System::AbilitySlot4), "restored loadout enables the bound slot on a new ship");
		restoredShip->Destroy();
		f.Remove(rModule);
		const auto rCell = restored.FindPlacement(rModule)->origin;
		report.Check(f.loadout.Move(f.rules, qModule, rCell, 0) == ModulePlacementFailure::None && f.ship->GetModuleApplier().Apply(f.loadout, f.rules), "moving an installed module preserves its binding");
		report.Check(f.loadout.FindPlacement(qModule)->abilitySlot == sas::AbilitySlot::Ability1 && !f.ship->GetAbilitySystemComponent().IsModuleSystemEnabled(GameplayTags::Module::System::AbilitySlot4), "Q remains Q after moving to R's cell");
		f.Remove(qModule);
		report.Check(f.loadout.Install(f.rules, qModule, rCell, 0) == ModulePlacementFailure::None && f.ship->GetModuleApplier().Apply(f.loadout, f.rules), "reinstalling in a different slot explicitly rebinds the module");
		report.Check(f.loadout.FindPlacement(qModule)->abilitySlot == sas::AbilitySlot::Ability4, "cargo to R chooses R binding");
	}

	void CascadeAndHaste(Report& report, Application& application)
	{
		Fixture f{ report, application };
		auto& rocket = Grant(f, RocketId, sas::AbilitySlot::Ability4);
		const auto baseline = Fire(f, rocket); const float damage = baseline->GetDamage(), cooldown = rocket.GetCooldownDuration(); baseline->Destroy();
		Replace(f, sas::AbilitySlot::Ability4);
		const auto cascade = f.Install("Module.Utility.CascadeReactor.Epic");
		const int effectiveLevel = 1 + ModuleResolver::FindZoneLevelBonus(f.ship->GetModuleApplier().GetResolvedEffects(), cascade);
		const float step = .20f + .02f * static_cast<float>(effectiveLevel - 1);
		CastMovement(f);
		const auto powered = Fire(f, rocket);
		f.Equal("Cascade and Ultimate multiply in the captured main effect", (1.f + step) * 1.6f, rocket.GetDefinition().mainEffectMultiplier);
		f.Equal("Cascade and Ultimate multiply actual Rocket damage", damage * (1.f + step) * 1.6f, powered->GetDamage());
		f.ship->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::AbilityHaste, 100.f);
		ScopedAbilityCooldownRule reduction; reduction.requiredSlot = sas::AbilitySlot::Ability4; reduction.multiplier = .75f;
		const auto handle = f.ship->GetAbilitySystemComponent().AddScopedAbilityCooldownRule(reduction);
		Fire(f, rocket);
		f.Equal("Ultimate multiplies haste and scoped cooldown reduction", cooldown * 2.f * sas::AttributeMath::GetAbilityCooldownMultiplier(100.f) * .75f, rocket.GetCooldownDuration());
		report.Check(f.ship->GetAbilitySystemComponent().RemoveScopedAbilityCooldownRule(handle), "remove only the E2E's scoped reduction");
		// Move the ability itself, leaving the module in R.
		report.Check(f.ship->GetAbilitySystemComponent().RemoveAbility(rocket.GetHandle()), "remove Rocket from its previous R slot before moving it");
		auto& moved = Grant(f, RocketId, sas::AbilitySlot::Ability1);
		Start(f, moved);
		f.Equal("ability moved to Q resolves Q's normal module", 1.f, moved.GetDefinition().mainEffectMultiplier);
	}

	void BarrierAndVoid(Report& report, Application& application)
	{
		Fixture f{ report, application };
		auto& barrier = Grant(f, "Ability.Defense.DirectionalBarrier.Basic", sas::AbilitySlot::Ability1);
		const float cooldown = barrier.GetCooldownDuration();
		Start(f, barrier); const float baseDuration = barrier.GetActiveDuration(); barrier.Cancel(sas::AbilityEndReason::Completed);
		const auto instance = Replace(f, sas::AbilitySlot::Ability1);
		const auto cascade = f.Install("Module.Utility.CascadeReactor.Epic");
		CastMovement(f);
		Start(f, barrier);
		const float duration = barrier.GetActiveDuration();
		f.Equal("Ultimate and Cascade leave Barrier duration unchanged", baseDuration, duration);
		report.Check(barrier.GetDefinition().mainEffectMultiplier > 1.6f, "Barrier cap is exercised with Ultimate multiplied by Cascade");
		const auto* effect = Effect(*f.ship, AbilityData::DirectionalBarrier::Effect::ActiveEffectId);
		report.Check(effect != nullptr, "Barrier creates its real effect snapshot");
		f.Equal("Ultimate Barrier stays capped at 180 degrees", 180.f, 2.f * sas::FindAttributeValue(effect->spec.attributes, AbilityData::DirectionalBarrier::Attribute::HalfAngleDegrees, 0.f));
		f.Remove(instance);
		f.Remove(cascade);
		f.Equal("OnEnd cooldown starts with the captured module after removal", cooldown * 2.f, barrier.GetCooldownRemaining());
		barrier.Tick(duration + .01f);
		f.Equal("OnEnd cooldown retains the cast's module snapshot", cooldown * 2.f, barrier.GetCooldownDuration());

		Fixture v{ report, application };
		auto& gate = Grant(v, "Ability.Utility.VoidGate.Basic", sas::AbilitySlot::Ability4);
		Start(v, gate); const float gateDuration = gate.GetActiveDuration();
		const float radius = Actors<VoidGatePortalActor>(v).front()->GetPortalRadius();
		AbilityExecutionContext execution{ &v.ship->GetAbilitySystemComponent(), &gate.GetDefinition(), nullptr, &gate };
		const auto values = AbilityActionAttributeResolver::ResolveAbilityAttributes(execution);
		gate.Cancel(sas::AbilityEndReason::Completed); gate.Tick(1000.f); const float gateCooldown = gate.GetCooldownDuration();
		Replace(v, sas::AbilitySlot::Ability4); Start(v, gate);
		v.Equal("Void Gate keeps its duration", gateDuration, gate.GetActiveDuration());
		report.Check(Actors<VoidGatePortalActor>(v).size() == 1, "Void Gate keeps one portal preview");
		v.Equal("Void Gate keeps its radius", radius, Actors<VoidGatePortalActor>(v).front()->GetPortalRadius());
		const auto powered = AbilityActionAttributeResolver::ResolveAbilityAttributes(execution);
		report.Check(powered.size() == values.size(), "Void Gate keeps exactly its authored runtime channels");
		for (const auto& value : values) report.CheckValue("Void Gate runtime values stay unchanged", value.currentValue, sas::FindAttributeValue(powered, value.id, -1.f), std::string{ value.id.GetName() });
		gate.Cancel(sas::AbilityEndReason::Completed);
		v.Equal("Void Gate only doubles cooldown", gateCooldown * 2.f, gate.GetCooldownDuration());
	}

	void HudAndRespawn(Report& report, Application& application)
	{
		auto& manager = PlayerManager::GetPlayerManager(); manager.Reset();
		struct Reset { PlayerManager& manager; ~Reset() { manager.Reset(); } } reset{ manager };
		auto level = std::make_shared<HeadlessModuleLevel>(&application);
		auto& player = manager.CreateNewPlayer();
		auto ship = player.SpawnSpaceShip(level.get()).lock();
		level->BeginPlayInternal(); level->TickInternal(0.f);
		report.Check(ship != nullptr, "real Player owns a ship and persistent run loadout");
		List<std::uint32_t> copies;
		for (const auto& item : player.GetModuleLoadout().GetInventory()) if (item.moduleId == sas::ContentId{ UltimateId }) copies.push_back(item.instanceId);
		report.Check(copies.size() == 4, "existing developer grant exposes four Ultimate copies");
		ModuleScreenPresenter presenter; presenter.ToggleOpen(); presenter.Tick(0.f);
		const auto model = presenter.GetViewModel();
		const auto cargo = std::find_if(model->inventory.begin(), model->inventory.end(), [&](const auto& entry) { return entry.instanceId == copies.front(); });
		report.Check(cargo != model->inventory.end() && cargo->name == "Ultimate Protocol" && cargo->rarity.empty() && cargo->levelCount == 0 && cargo->family == "Ability" && cargo->sideFeatureState == ModuleSideFeatureViewState::None, "HUD cargo shows Ultimate without rarity level or side feature");
		ModulePlacement shell;
		for (const auto& placement : player.GetModuleLoadout().GetPlacements()) if (placement.moduleId == sas::ContentId{ "Module.Utility.AbilitySlot4.Standard" }) shell = placement;
		report.Check(shell.instanceId != 0 && player.TryUninstallModule(shell.instanceId), "Player uninstalls the R standard shell");
		presenter.PickUpInventory(copies.front());
		presenter.Tick(0.f);
		report.Check(model->heldInstanceId == copies.front(), "HUD picks up Ultimate from cargo");
		presenter.SetHoverCell(shell.origin); presenter.Tick(0.f);
		report.Check(model->preview.legal && model->preview.zoneLevelBonus == 0, "HUD previews a legal replacement without zone scaling");
		report.Check(presenter.DropOnGrid(), "HUD installs Ultimate through Player"); presenter.Tick(0.f);
		const auto installed = std::find_if(model->installed.begin(), model->installed.end(), [&](const auto& entry) { return entry.instanceId == copies.front(); });
		report.Check(installed != model->installed.end() && installed->name == "Ultimate Protocol (R)" && installed->rarity.empty() && installed->levelCount == 0, "HUD installed entry preserves Ultimate's name and displays R binding");
		ship->Destroy(); level->TickInternal(0.f);
		const auto respawn = player.SpawnSpaceShip(level.get()).lock(); level->TickInternal(0.f);
		report.Check(respawn && player.GetModuleLoadout().FindPlacement(copies.front())->abilitySlot == sas::AbilitySlot::Ability4 && respawn->GetAbilitySystemComponent().IsModuleSystemEnabled(GameplayTags::Module::System::AbilitySlot4), "Player respawn restores Ultimate binding and enables R");
		report.Check(respawn->GetModuleApplier().GetResolvedEffects().abilityModules.size() == 1, "respawn reapplies exactly the installed ability module");
	}

	void ContentAndFamilies(Report& report, Application& application)
	{
		const auto& module = *content::ModuleContentCatalog::FindById(UltimateId);
		report.Check(module.rarity == ModuleRarity::None && !module.family.IsValid() && module.sideFeatures.empty() && module.sharedPools.empty() && module.sizeVariants.size() == 1 && module.sizeVariants.front().shape.cells.size() == 1, "Ultimate is a familyless rarityless sideless one-cell module");
		report.Check(module.displayName == "Ultimate Protocol" && module.description.find("{abilityModule.mainEffectMultiplier}") != std::string::npos, "English content uses numeric placeholders");
		const auto text = FormatModuleDescription(module, module.sizeVariants.front(), 1);
		report.Check(text.find('{') == std::string::npos && text.find("1,6") != std::string::npos && text.find("x2") != std::string::npos, "production formatter resolves both JSON values");
		for (const auto rarity : { ModuleRarity::Common, ModuleRarity::Uncommon, ModuleRarity::Rare, ModuleRarity::Epic, ModuleRarity::Legendary })
		{
			List<sas::ContentId> pool;
			for (const auto& candidate : content::ModuleContentCatalog::GetDefinitions()) if (candidate.rarity == rarity) pool.push_back(candidate.moduleId);
			report.Check(std::find(pool.begin(), pool.end(), module.moduleId) == pool.end(), "Ultimate is absent from every rarity-selected loot pool", static_cast<int>(rarity));
		}
		Fixture f{ report, application };
		f.Install("Module.Utility.ConfluenceCore.Epic");
		const auto before = f.ship->GetModuleApplier().GetResolvedEffects();
		for (const auto slot : { sas::AbilitySlot::Ability1, sas::AbilitySlot::Ability2, sas::AbilitySlot::Ability3, sas::AbilitySlot::Ability4 }) Replace(f, slot);
		const auto& after = f.ship->GetModuleApplier().GetResolvedEffects();
		report.Check(before.ownerModifiers.size() == after.ownerModifiers.size() && before.abilityRules.size() == after.abilityRules.size() && before.primaryWeaponModifiers.size() == after.primaryWeaponModifiers.size(), "four Ultimate modules add no Confluence family contributions");
		for (std::size_t i = 0; i < before.ownerModifiers.size(); ++i) f.Equal("Confluence owner values unchanged", before.ownerModifiers[i].magnitude, after.ownerModifiers[i].magnitude);
		std::ifstream input{ std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/modules.json" };
		Json source; input >> source;
		auto& authored = *std::find_if(source["modules"].begin(), source["modules"].end(), [](const Json& entry) { return entry.at("id") == UltimateId; });
		const auto original = authored;
		for (const auto& field : { "rarity", "levelCount", "sideFeatures", "sharedPools", "family", "systemCategory", "sizeLadder" })
		{
			authored = original; authored[field] = field == std::string{ "rarity" } ? Json{ "Epic" } : Json{ 1 };
			report.Check(!content::ModuleLoader::LoadFromJson(source, UltimateId).Succeeded(), std::string{ "loader rejects ability module " } + field);
		}
		authored = original; authored["sizeVariants"][0]["shape"] = Json::array({ Json::array({ 0, 0 }), Json::array({ 1, 0 }) });
		report.Check(!content::ModuleLoader::LoadFromJson(source, UltimateId).Succeeded(), "loader rejects multi-cell ability modules");
		authored = original; authored["abilityModule"]["mainEffectMultiplier"] = Json::array({ 1.6, 2.0, 2.5 });
		report.Check(!content::ModuleLoader::LoadFromJson(source, UltimateId).Succeeded(), "loader rejects leveled ability multipliers");
		authored = original; authored["sizeVariants"][0]["zoneEffects"] = Json::array();
		report.Check(!content::ModuleLoader::LoadFromJson(source, UltimateId).Succeeded(), "loader rejects zone rules on ability modules");
	}
}

int main(int argc, char** argv)
{
	vega::EngineContext context; vega::EngineContext::Scope scope{ context }; vega::InstallVegaEngineAssets();
	if (argc != 2) return 2;
	Report report;
	report.artifact["test"] = "AbilityModuleUltimateProtocolE2E";
	try
	{
		context.Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Ultimate Protocol content registration failed" };
		std::string title = "Ability Module Ultimate Protocol E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None }; application.GetRenderWindow().setVisible(false);
		ContentAndFamilies(report, application); RocketAndRemoval(report, application); SlotsAndCounts(report, application); CascadeAndHaste(report, application); BarrierAndVoid(report, application); HudAndRespawn(report, application);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& error) { report.artifact["error"] = error.what(); std::cerr << error.what() << '\n'; }
	WriteArtifact(argv[1], report.artifact); return report.artifact.value("passed", false) ? 0 : 1;
}
