#include "framework/EngineContext.h"
#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP6E2E.h"
#include "gameplay/ability/rocket/RocketProjectileActor.h"
#include "gameplay/ability/energySpear/EnergySpearTraversalActor.h"
#include "gameplay/ability/runtime/MainEffectMultiplier.h"
#include "gameplay/damage/DamageTypeOverride.h"
#include "gameplay/tags/GameplayTagSchema.h"
#include "presentation/hud/ability/AbilityBarPresenter.h"
#include "presentation/hud/ability/AbilityBarView.h"
#include "presentation/hud/modules/ModuleScreenView.h"
#include "player/Player.h"
#include "player/PlayerManager.h"
#include "enemy/EnemyActor.h"
#include "gameplay/content/EnemyFactory.h"
#include "gameplay/enemy/EnemyIds.h"
#include <random>
#include <set>

namespace
{
	using namespace uncommon_p6_e2e;
	constexpr const char* Timing = "Module.AbilityModule.TimingProtocol";
	constexpr const char* Autocast = "Module.AbilityModule.AutocastProtocol";
	constexpr const char* Element = "Module.AbilityModule.ElementCore";
	constexpr const char* Fortune = "Module.AbilityModule.FortuneProtocol";
	constexpr const char* Rocket = "Ability.Offense.Rocket.Basic";
	constexpr auto Q = sas::AbilitySlot::Ability1;
	constexpr auto E = sas::AbilitySlot::Ability2;
	constexpr auto R = sas::AbilitySlot::Ability4;

	GameAbility& Grant(Fixture& f, const char* id, sas::AbilitySlot slot = Q)
	{
		auto& asc = f.ship->GetAbilitySystemComponent();
		if (const auto* current = asc.GetAbility(slot)) f.report.Check(asc.RemoveAbility(current->GetHandle()), "replace selected slot through ASC");
		if (const auto* current = asc.GetAbilityById(id)) f.report.Check(asc.RemoveAbility(current->GetHandle()), "move existing content without duplicate ability ID");
		const auto* authored = content::AbilityContentCatalog::FindById(id);
		f.report.Check(authored != nullptr, std::string{ "shipped ability: " } + id);
		auto definition = *authored; definition.slot = slot;
		f.report.Check(asc.GrantAbility(definition).IsValid(), "grant real ability with runtime slot binding");
		return *asc.GetAbility(slot);
	}
	ModulePlacement Shell(Fixture& f, sas::AbilitySlot slot)
	{
		for (const auto& placement : f.loadout.GetPlacements()) if (AbilitySlotForModuleSystem(f.rules.lookup(placement.moduleId)->systemCategory) == slot) return placement;
		throw std::runtime_error{ "standard shell not found" };
	}
	std::uint32_t Replace(Fixture& f, const char* id, sas::AbilitySlot slot = Q, GameplayTag element = {})
	{
		const auto shell = Shell(f, slot); f.Remove(shell.instanceId);
		const auto& definition = *f.rules.lookup(id);
		const auto instance = f.loadout.AddToInventory(f.rules, definition, 0, definition.rarity, 1);
		if (element.IsValid()) f.report.Check(f.loadout.SetInventoryElement(f.rules, instance, element), "developer element choice validates canonical pool");
		f.report.Check(instance != 0 && f.loadout.Install(f.rules, instance, shell.origin, 0) == ModulePlacementFailure::None && f.ship->GetModuleApplier().Apply(f.loadout, f.rules), "install ability module through loadout and ship applier");
		f.report.Check(f.loadout.FindPlacement(instance)->abilitySlot == slot, "module captures selected key");
		return instance;
	}
	void Ready(GameAbility& ability)
	{
		if (ability.IsActive()) ability.Cancel(sas::AbilityEndReason::Completed);
		ability.SetInputHeld(false); ability.Tick(1000.f);
	}
	void Key(Fixture& f, sas::AbilitySlot slot, bool held)
	{
		auto& asc = f.ship->GetAbilitySystemComponent(); asc.SetLoadoutKeyInput(slot, held, false); asc.Tick(0.f);
	}
	shared_ptr<RocketProjectileActor> LatestRocket(Fixture& f)
	{
		const auto actors = Actors<RocketProjectileActor>(f); f.report.Check(!actors.empty(), "activation spawns a real Rocket"); return *std::max_element(actors.begin(), actors.end(), [](const auto& a, const auto& b) { return a->GetUniqueID() < b->GetUniqueID(); });
	}
	struct HitCapture
	{
		SpaceShip& owner; DelegateHandle token; List<GameplayTag> types; DamagePayload payload; float damage = 0.f;
		explicit HitCapture(SpaceShip& ship) : owner{ ship }, token{ ship.GetCombatRuntime().onDamageDealt.BindAction(this, &HitCapture::Observe) } {}
		~HitCapture() { owner.GetCombatRuntime().onDamageDealt.UnbindAction(token); }
		void Observe(const DamageContext& context) { types = context.damageTags; payload = context.payload; damage = context.appliedDamage; }
	};
	void Content(Report& report)
	{
		std::ifstream input{ std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/modules.json" }; Json source; input >> source;
		for (const auto* id : { Timing, Autocast, Element, Fortune })
		{
			const auto& definition = *content::ModuleContentCatalog::FindById(id);
			report.Check(definition.abilityModule && !definition.unique && !definition.family.IsValid() && !definition.systemCategory.IsValid() && definition.rarity == ModuleRarity::None && definition.levelCount == 1 && definition.sideFeatures.empty() && definition.sharedPools.empty() && definition.sizeLadderCellCounts.empty() && definition.sizeVariants.size() == 1 && definition.sizeVariants.front().shape.cells.size() == 1 && definition.sizeVariants.front().zoneEffects.empty(), std::string{ "rarityless one-cell class contract: " } + id);
			report.Check(FormatModuleDescription(definition, definition.sizeVariants.front(), 1).find('{') == std::string::npos, "English descriptions resolve all typed JSON values");
			auto& authored = *std::find_if(source["modules"].begin(), source["modules"].end(), [id](const Json& entry) { return entry.at("id") == id; }); const auto original = authored;
			for (const char* forbidden : { "rarity", "family", "levelCount", "sideFeatures", "sharedPools", "sizeLadder", "systemCategory" }) { authored = original; authored[forbidden] = 1; report.Check(!content::ModuleLoader::LoadFromJson(source, id).Succeeded(), "loader rejects forbidden ability-module class field"); }
			authored = original; authored["sizeVariants"][0]["zoneEffects"] = Json::array(); report.Check(!content::ModuleLoader::LoadFromJson(source, id).Succeeded(), "loader rejects zone effects");
			authored = original; authored["abilityModule"]["unknown"] = 1; report.Check(!content::ModuleLoader::LoadFromJson(source, id).Succeeded(), "loader rejects unknown typed rule");
			authored = original;
			if (definition.abilityModule->timing) authored["abilityModule"]["timing"]["smallWindowSeconds"] = 2.f;
			if (definition.abilityModule->autocast) authored["abilityModule"]["autocast"]["defaultRange"] = -1.f;
			if (definition.abilityModule->elementCore) authored["abilityModule"]["elementCore"]["elements"][0] = "Damage.Type.Photonic";
			if (definition.abilityModule->fortune) authored["abilityModule"]["fortune"]["minimum"] = 2.f;
			report.Check(!content::ModuleLoader::LoadFromJson(source, id).Succeeded(), "loader validates each module's numeric or element invariant"); authored = original;
		}
		report.Check(content::ModuleLoader::LoadFromJson(source, "ability modules restored").Succeeded(), "valid content survives malformed-content checks");
	}
	void TimingChecks(Report& report, Application& app)
	{
		Fixture f{ report, app }; auto& rocket = Grant(f, Rocket); auto& asc = f.ship->GetAbilitySystemComponent();
		report.Check(rocket.TryActivate(), "baseline Rocket activation"); const float damage = LatestRocket(f)->GetDamage(), cooldown = rocket.GetCooldownDuration(); Ready(rocket);
		const auto instance = Replace(f, Timing);
		for (const auto& sample : List<std::tuple<float, float, float, std::string>>{ { .84f, 1.4f, .85f, "PERFECT" }, { .66f, 1.15f, .95f, "GOOD" }, { .2f, .85f, 1.4f, "MISS" }, { 1.21f, .85f, 1.4f, "MISS" } })
		{
			const auto& [seconds, power, cd, feedback] = sample;
			Ready(rocket); Key(f, Q, false); const auto count = asc.GetLoadoutAbilityActivationCount(); Key(f, Q, true);
			report.Check(asc.GetActivationPolicy().GetView(Q).open && asc.GetLoadoutAbilityActivationCount() == count, "first press opens bar without activating or spending cooldown");
			Key(f, Q, false); asc.Tick(seconds);
			if (seconds < 1.2f) { report.Check(asc.GetActivationPolicy().GetView(Q).open, "bar persists until second press"); Key(f, Q, true); }
			report.Check(asc.GetLoadoutAbilityActivationCount() == count + 1 && !asc.GetActivationPolicy().GetView(Q).open, "second press or timeout casts exactly once");
			f.Equal("Timing captures selected main effect", power, rocket.GetDefinition().mainEffectMultiplier); f.Equal("Timing scales actual projectile", damage * power, LatestRocket(f)->GetDamage());
			f.Equal("Timing captures cooldown", cooldown * cd, rocket.GetCooldownDuration()); f.Equal("Timing starts selected cooldown", cooldown * cd, rocket.GetCooldownRemaining());
			report.Check(asc.GetActivationPolicy().GetView(Q).feedback == feedback, "Timing HUD feedback identifies the exclusive result");
		}
		Key(f, Q, false); const auto count = asc.GetLoadoutAbilityActivationCount(); Key(f, Q, true);
		report.Check(!asc.GetActivationPolicy().GetView(Q).open && asc.GetLoadoutAbilityActivationCount() == count, "cooldown key preserves existing rejection");
		Ready(rocket); Key(f, Q, false); asc.AddOwnedTag(GameplayTagSchema::BlockAbilityActivation); Key(f, Q, true);
		report.Check(!asc.GetActivationPolicy().GetView(Q).open && asc.GetLoadoutAbilityActivationCount() == count, "locked ready key does not open bar"); asc.RemoveOwnedTag(GameplayTagSchema::BlockAbilityActivation); Key(f, Q, false);
		auto& shield = Grant(f, "Ability.Defense.Shield.Basic", E); Replace(f, Timing, E);
		Key(f, Q, true); Key(f, Q, false); Key(f, E, true); Key(f, E, false);
		report.Check(asc.GetActivationPolicy().GetView(Q).open && asc.GetActivationPolicy().GetView(E).open, "opening another Timing key leaves both independent bars open");
		auto& normal = Grant(f, "Ability.Utility.VoidGate.Basic", R); report.Check(normal.TryActivate(), "unrelated key remains usable while two bars are open");
		asc.Tick(.84f); Key(f, E, true); f.Equal("second Timing key scores independently", 1.4f, shield.GetDefinition().mainEffectMultiplier);
		f.Remove(instance); report.Check(!asc.GetActivationPolicy().GetView(Q).open, "removing Timing cancels only its pending bar"); f.Equal("unrelated module rebuild preserves active Timing power", 1.4f, shield.GetDefinition().mainEffectMultiplier);
		Fixture combo{ report, app }; auto& combined = Grant(combo, Rocket); Replace(combo, Timing); const auto cascade = combo.Install("Module.Utility.CascadeReactor.Epic"); CastMovement(combo); Key(combo, Q, true); Key(combo, Q, false); combo.ship->GetAbilitySystemComponent().Tick(.84f); Key(combo, Q, true);
		const float step = .20f + .02f * static_cast<float>(ModuleResolver::FindZoneLevelBonus(combo.ship->GetModuleApplier().GetResolvedEffects(), cascade)); combo.Equal("Timing and Cascade share the additive main-effect group", 1.4f + step, combined.GetDefinition().mainEffectMultiplier);
	}
	void AutocastChecks(Report& report, Application& app)
	{
		Fixture f{ report, app }; auto& rocket = Grant(f, Rocket); auto& asc = f.ship->GetAbilitySystemComponent(); Replace(f, Autocast);
		const auto start = asc.GetLoadoutAbilityActivationCount(); asc.Tick(0.f); report.Check(asc.GetLoadoutAbilityActivationCount() == start, "Offense waits without a valid enemy");
		const float range = asc.GetActivationPolicy().ResolveAutocastRange(rocket, 600.f); f.Equal("Rocket range comes from actor data", 1100.f, range);
		const auto distant = Enemy(f, range + 10.f); asc.Tick(0.f); report.Check(asc.GetLoadoutAbilityActivationCount() == start, "Offense waits outside authored range");
		asc.AddOwnedTag(GameplayTagSchema::BlockAbilityActivation); const auto near = Enemy(f, 200.f); const auto far = Enemy(f, 400.f); near->SetActorLocation(f.ship->GetActorLocation() + sf::Vector2f{ 0.f, 200.f }); asc.RemoveOwnedTag(GameplayTagSchema::BlockAbilityActivation); asc.Tick(0.f);
		report.Check(asc.GetLoadoutAbilityActivationCount() == start + 1 && rocket.GetActivationAim().has_value(), "ready Autocast fires automatically");
		f.Equal("Autocast captured power", .75f, rocket.GetDefinition().mainEffectMultiplier);
		f.Equal("nearest enemy is the snapshotted target", near->GetActorLocation().y, rocket.GetActivationAim()->targetPoint.y);
		report.Check(LatestRocket(f)->GetActorForwardDirection().y > .99f, "actual projectile aims at nearest enemy instead of cursor");
		const auto count = asc.GetLoadoutAbilityActivationCount(); asc.Tick(.1f); report.Check(asc.GetLoadoutAbilityActivationCount() == count, "Autocast respects running cooldown");
		near->Destroy(); far->Destroy(); distant->Destroy(); Ready(rocket); Key(f, Q, true); f.Equal("manual Autocast key uses same power", .75f, rocket.GetDefinition().mainEffectMultiplier); report.Check(!rocket.GetActivationAim(), "manual key restores cursor aim"); Key(f, Q, false);
		Ready(rocket); const auto target = Enemy(f, 200.f);
		asc.AddOwnedTag(GameplayTagSchema::BlockAbilityActivation); const auto lockedCount = asc.GetLoadoutAbilityActivationCount(); asc.Tick(0.f); report.Check(asc.GetLoadoutAbilityActivationCount() == lockedCount, "Autocast never activates under shared ability lock"); asc.RemoveOwnedTag(GameplayTagSchema::BlockAbilityActivation);
		f.ship->BeginPortalTransit(); asc.Tick(0.f); report.Check(asc.GetLoadoutAbilityActivationCount() == lockedCount, "Autocast never activates in portal transit"); f.ship->CompletePortalTransit(f.ship->GetActorLocation());
		EnemySpawnContext spawnContext; spawnContext.windowCullEnabled = false;
		const auto host = content::SpawnEnemy(*f.level, EnemyIds::ApproachGunnerBasic, f.ship->GetActorLocation() + sf::Vector2f{ 300.f, 0.f }, 1.f, spawnContext).lock();
		report.Check(host && f.ship->BeginPossession(*host), "enter possession through real ship controller"); asc.Tick(0.f); report.Check(asc.GetLoadoutAbilityActivationCount() == lockedCount, "Autocast never activates while possessing"); f.ship->EndPossession(f.ship->GetActorLocation()); host->Destroy();
		f.ship->GetHealthComponent().SetInitialHealth(0.f, 1000.f); asc.Tick(0.f); report.Check(asc.GetLoadoutAbilityActivationCount() == lockedCount, "Autocast never activates with zero health"); f.ship->GetHealthComponent().SetInitialHealth(1000.f, 1000.f); target->Destroy();

		Fixture utility{ report, app }; auto& bola = Grant(utility, "Ability.Utility.CryoBola.Basic"); auto& ua = utility.ship->GetAbilitySystemComponent(); Replace(utility, Autocast); ua.Tick(0.f); report.Check(!bola.IsOnCooldown(), "Utility waits without enemies"); Enemy(utility, 100.f); ua.Tick(0.f); report.Check(bola.IsOnCooldown(), "Utility fires with an in-range enemy");
		Fixture defense{ report, app }; auto& shield = Grant(defense, "Ability.Defense.Shield.Basic"); auto& da = defense.ship->GetAbilitySystemComponent(); Replace(defense, Autocast); da.Tick(0.f); report.Check(!shield.IsActive(), "Defense waits without damage or enemies"); const auto farTarget = Enemy(defense, 1500.f); Hit(*farTarget, defense.ship.get(), 1.f, Plain()); da.Tick(0.f); report.Check(!shield.IsActive(), "dealing damage does not satisfy the defensive damage-taken condition"); farTarget->Destroy(); defense.ship->ApplyDamage(1.f); da.Tick(0.f); report.Check(shield.IsActive(), "Defense reacts to real damage within one second"); Ready(shield); da.Tick(1.01f); report.Check(!shield.IsActive(), "Defense damage window expires"); Enemy(defense, 100.f); da.Tick(0.f); report.Check(shield.IsActive(), "Defense also fires for nearby enemy");
		Fixture movement{ report, app }; auto& spear = Grant(movement, "Ability.Movement.EnergySpear.Basic"); auto& ma = movement.ship->GetAbilitySystemComponent(); Replace(movement, Autocast); Enemy(movement, 100.f); ma.Tick(0.f); report.Check(!spear.IsActive(), "Movement ignores nearby enemies without held movement input"); movement.ship->FindComponent<PlayerMovementComponent>()->ApplyInput({ 1.f, 0.f }, false, 0.f); ma.Tick(0.f);
		report.Check(spear.IsActive() && spear.GetActivationAim() && spear.GetActivationAim()->direction.x > .99f, "Movement starts toward held world movement input");
		f.Equal("charged movement snapshots reduced main effect", .75f, spear.GetDefinition().mainEffectMultiplier);
		ma.SetLoadoutKeyInput(Q, false, false); ma.Tick(.7f); report.Check(spear.IsActive() && Actors<EnergySpearTraversalActor>(movement).empty(), "virtual hold survives physical key release and does not release early"); ma.Tick(.8f);
		report.Check(!Actors<EnergySpearTraversalActor>(movement).empty(), "hold ability reaches full charge and releases into real traversal");
	}
	void ElementChecks(Report& report, Application& app)
	{
		Fixture f{ report, app }; auto& rocket = Grant(f, Rocket); auto& asc = f.ship->GetAbilitySystemComponent(); const auto& definition = *f.rules.lookup(Element);
		f.loadout.SetRandomSeed(481); auto copy = f.loadout;
		std::set<std::string> seen;
		for (int i = 0; i < 30; ++i) { const auto a = f.loadout.AddToInventory(f.rules, definition, 0, definition.rarity, 1), b = copy.AddToInventory(f.rules, definition, 0, definition.rarity, 1); const auto element = f.loadout.FindInventoryItem(a)->element; seen.insert(element.name); report.Check(element.IsValid() && element != DamageTypeSchema::Photonic && element == copy.FindInventoryItem(b)->element, "acquisition RNG is seedable and stores an element on each instance"); }
		report.Check(seen.size() == 5, "acquisition pool contains all five elements");
		for (const auto& element : definition.abilityModule->elementCore->elements)
		{
			Fixture hit{ report, app }; auto& cast = Grant(hit, Rocket); auto& ha = hit.ship->GetAbilitySystemComponent(); const auto module = Replace(hit, Element, Q, element);
			report.Check(cast.TryActivate(), "Element Rocket activates"); auto projectile = LatestRocket(hit); const auto captured = projectile->GetDamagePayload().hitGroup; report.Check(captured && captured->damageTypeOverride == element, "projectile owns the activation's conversion snapshot");
			const auto target = Enemy(hit, 200.f); target->SetCollisionMask(GameCollision::PlayerBullet); auto payload = projectile->GetDamagePayload(); payload.criticalPolicy = DamageCriticalPolicy::Disabled; projectile->SetDamagePayload(payload); projectile->SetActorLocation(target->GetActorLocation()); hit.level->TickInternal(0.f);
			hit.Remove(module); HitCapture observer{ *hit.ship }; projectile->OnActorBeginOverlap(target.get());
			report.Check(observer.types == List<GameplayTag>{ element } && observer.damage > 0.f, "real projectile converts type before damage readers even after module removal");
			if (element == DamageTypeSchema::Energy) report.Check(observer.payload.igniteStacks == 0 && observer.payload.cryoBuildupPerHit == 0 && observer.payload.electricStacks == 0 && observer.payload.kineticStacks == 0 && !target->GetAbilitySystemComponent().HasAnyOwnedTags({ DamageStatusSchema::Ignite, DamageStatusSchema::CryoSlowed, DamageStatusSchema::Electric, DamageStatusSchema::Kinetic }), "Energy suppresses every damage status");
			else
			{
				report.Check((element == DamageTypeSchema::Thermal && observer.payload.igniteStacks == 2) || (element == DamageTypeSchema::Cryo && observer.payload.cryoBuildupPerHit == 2) || (element == DamageTypeSchema::Electric && observer.payload.electricStacks == 2) || (element == DamageTypeSchema::Kinetic && observer.payload.kineticStacks == 2), "converted status carries original stack count");
				const char* effect = element == DamageTypeSchema::Thermal ? DamageStatusEffectIds::IgniteEffectId : element == DamageTypeSchema::Cryo ? DamageStatusEffectIds::CryoSlowedEffectId : element == DamageTypeSchema::Electric ? DamageStatusEffectIds::ElectricEffectId : DamageStatusEffectIds::KineticEffectId;
				report.Check(Stacks(*target, effect) == 2, "converted damage applies the real element status effects");
			}
		}
		Fixture reader{ report, app }; auto& shot = Grant(reader, Rocket); Grant(reader, "Ability.Utility.CryoBola.Basic", E); reader.Install("Module.Offense.MonochromeCore.Epic"); Replace(reader, Element, Q, DamageTypeSchema::Cryo);
		const auto& effects = reader.ship->GetModuleApplier().GetResolvedEffects().epicOffense; report.Check(std::any_of(effects.monochromeMultipliers.begin(), effects.monochromeMultipliers.end(), [](const auto& entry) { return entry.first == DamageTypeSchema::Cryo; }), "Monochrome loadout reader counts converted type");
		const auto target = Enemy(reader, 300.f); HitCapture observer{ *reader.ship }; report.Check(shot.TryActivate(), "converted Monochrome activation"); auto payload = Plain(); payload.hitGroup = shot.GetDamageHitGroup(); const float delivered = Hit(*target, reader.ship.get(), 100.f, payload, shot.GetDefinition().abilityId, shot.GetDefinition().abilityTags, DamageDeliveryType::Area, { DamageTypeSchema::Kinetic });
		report.Check(observer.types == List<GameplayTag>{ DamageTypeSchema::Cryo } && delivered > 100.f, "outgoing Monochrome damage modifier sees the converted hit");
		Fixture harmless{ report, app }; auto& shield = Grant(harmless, "Ability.Defense.Shield.Basic"); report.Check(shield.TryActivate(), "non-damaging baseline"); const auto* baseline = Effect(*harmless.ship, "Effect.Barrier.Basic"); report.Check(baseline != nullptr, "shield creates real barrier effect"); const auto baselineValues = baseline->spec.attributes; Ready(shield); Replace(harmless, Element, Q, DamageTypeSchema::Thermal); report.Check(shield.TryActivate(), "non-damaging ability accepts Element Core"); const auto* converted = Effect(*harmless.ship, "Effect.Barrier.Basic"); report.Check(converted && converted->spec.attributes.size() == baselineValues.size(), "non-damaging ability keeps its authored effect channels");
		for (const auto& value : baselineValues) report.CheckValue("non-damaging ability values are unchanged", value.currentValue, sas::FindAttributeValue(converted->spec.attributes, value.id, -1.f), std::string{ value.id.GetName() });
	}
	void FortuneChecks(Report& report, Application& app)
	{
		Fixture f{ report, app }; auto& rocket = Grant(f, Rocket); auto& asc = f.ship->GetAbilitySystemComponent(); const float cooldown = rocket.GetCooldownDuration(); Replace(f, Fortune);
		const auto& spec = *f.rules.lookup(Fortune)->abilityModule->fortune;
		for (const auto luck : { -2.f, 0.f, 1.f, 20.f, 100.f })
		{
			Ready(rocket); asc.GetAttributes().SetBaseValue(OwnerAttributeIds::Luck, luck); asc.GetActivationPolicy().SetRandomSeed(1729);
			std::mt19937 random{ 1729 }; std::uniform_real_distribution<float> roll{ spec.minimum, spec.maximum }, chance{ 0.f, 1.f }; float expected = roll(random); if (chance(random) < std::clamp(spec.doubleRollChancePerLuck * luck, 0.f, 1.f)) expected = std::max(expected, roll(random));
			report.Check(rocket.TryActivate(), "Fortune uses real activation"); const float value = rocket.GetDefinition().mainEffectMultiplier;
			report.Check(value >= .5f && value <= 1.7f, "Fortune stays within uniform roll interval"); f.Equal("Fortune seeded roll and clamped Luck double-roll", expected, value); f.Equal("Fortune cooldown is unchanged", cooldown, rocket.GetCooldownDuration());
			report.Check(asc.GetActivationPolicy().GetView(Q).feedback.rfind("x", 0) == 0, "Fortune publishes rolled value on ability icon");
		}
		Ready(rocket); asc.GetActivationPolicy().SetRandomSeed(31); report.Check(rocket.TryActivate(), "Fortune snapshot activation"); const auto previous = LatestRocket(f); const float captured = previous->GetDamage(); Ready(rocket); report.Check(rocket.TryActivate(), "next Fortune activation rolls independently"); f.Equal("earlier projectile retains materialized damage", captured, previous->GetDamage());
		const auto record = asc.GetAbilityUseHistory().GetRecords().front(); const float replayDamage = LatestRocket(f)->GetDamage();
		report.Check(asc.InvokeRecordedAbility(record, rocket.GetDefinition().scalingRules, 1.f, Q, false), "recorded invocation uses the committed activation snapshot");
		const auto replay = LatestRocket(f); report.artifact["fortuneReplay"] = { { "recordPower", record.mainEffectMultiplier }, { "recordGroup", record.mainEffectGroupMultiplier }, { "recordCaptured", record.activationModifiersCaptured }, { "replayPower", replay->GetDamagePayload().mainEffectMultiplier }, { "level", record.level } };
		f.Equal("Fortune replay preserves its original roll", replayDamage, replay->GetDamage());
	}
	void PlacementAndHud(Report& report, Application& app)
	{
		for (const auto* id : { Timing, Autocast, Element, Fortune })
		{
			Fixture f{ report, app }; const auto q = Replace(f, id); const auto r = Shell(f, R); f.Remove(r.instanceId); const auto original = *f.loadout.FindPlacement(q);
			report.Check(f.loadout.Move(f.rules, q, r.origin, 0) == ModulePlacementFailure::None && f.loadout.FindPlacement(q)->abilitySlot == Q && f.loadout.FindPlacement(q)->element == original.element, "move preserves key and stored element");
			auto copied = f.loadout; const auto ship = f.level->SpawnActor<PlayerSpaceShip>(content::ShipContentCatalog::GetPlayerFighterDefinition()).lock(); report.Check(ship && ship->GetModuleApplier().Apply(copied, f.rules) && ship->GetAbilitySystemComponent().IsModuleSystemEnabled(ModuleSystemForAbilitySlot(Q)) && copied.FindPlacement(q)->element == original.element, "loadout copy onto respawn preserves binding and element"); ship->Destroy();
			f.Equal("ability module never gains zone level", 0.f, static_cast<float>(ModuleResolver::FindZoneLevelBonus(f.ship->GetModuleApplier().GetResolvedEffects(), q)));
			report.Check(!f.loadout.SetLevel(f.rules, q, 2) && !f.loadout.SetSideFeature(f.rules, q, "Module.SideFeature.FieldMastery.KnowledgeTransfer"), "no level or side feature can be assigned");
			auto duplicate = original; duplicate.instanceId += 10000;
			report.Check(ModulePlacementRules::CanPlace(f.rules, List<ModulePlacement>{ *f.loadout.FindPlacement(q) }, duplicate) == ModulePlacementFailure::DuplicateSystem, "same key cannot carry a second ability module in a different cell");
		}
		auto& manager = PlayerManager::GetPlayerManager(); manager.Reset(); struct Reset { PlayerManager& manager; ~Reset() { manager.Reset(); } } reset{ manager };
		auto level = std::make_shared<HeadlessModuleLevel>(&app); auto& player = manager.CreateNewPlayer(); auto ship = player.SpawnSpaceShip(level.get()).lock(); level->BeginPlayInternal(); level->TickInternal(0.f);
		ModuleScreenPresenter presenter; presenter.ToggleOpen(); presenter.Tick(0.f); const auto model = presenter.GetViewModel(); std::set<std::string> elements;
		for (const auto* id : { Timing, Autocast, Element, Fortune })
		{
			int count = 0;
			for (const auto& item : player.GetModuleLoadout().GetInventory()) if (item.moduleId == sas::ContentId{ id })
			{
				++count; const auto entry = std::find_if(model->inventory.begin(), model->inventory.end(), [&](const auto& view) { return view.instanceId == item.instanceId; });
				report.Check(entry != model->inventory.end() && entry->category == "Unique" && entry->subKind == "Ability module" && entry->rarity.empty() && entry->levelCount == 0, "ability modules share Unique UI category with explicit sub-kind");
				if (item.element.IsValid()) { elements.insert(item.element.name); report.Check(entry->name.find(item.element.name.substr(item.element.name.rfind('.') + 1)) != std::string::npos && entry->description.find("Stored element:") != std::string::npos, "module screen name and tooltip show stored element"); }
			}
			report.Check(count == (id == Element ? 5 : 1), "developer inventory grants requested copy count");
		}
		report.Check(elements.size() == 5, "developer inventory exposes one Element Core of every element");
		report.Check(std::any_of(model->inventory.begin(), model->inventory.end(), [](const auto& entry) { return entry.category == "Unique" && entry.subKind == "Connector Unique"; }), "connector Uniques keep their separate sub-kind in the shared category");
		std::uint32_t timingId = 0, elementId = 0; GameplayTag stored;
		for (const auto& item : player.GetModuleLoadout().GetInventory()) { if (item.moduleId == Timing) timingId = item.instanceId; if (item.moduleId == Element) { elementId = item.instanceId; stored = item.element; } }
		ModulePlacement qShell, rShell;
		for (const auto& placement : player.GetModuleLoadout().GetPlacements()) { if (placement.moduleId == "Module.Utility.AbilitySlot1.Standard") qShell = placement; if (placement.moduleId == "Module.Utility.AbilitySlot4.Standard") rShell = placement; }
		report.Check(player.TryUninstallModule(qShell.instanceId) && player.TryInstallModule(timingId, qShell.origin, 0) == ModulePlacementFailure::None, "Player installs developer Timing copy");
		auto& asc = ship->GetAbilitySystemComponent(); asc.SetLoadoutKeyInput(Q, true, false); asc.Tick(.84f);
		AbilityBarPresenter bar; bar.Tick(); const auto barModel = bar.GetViewModel();
		report.Check(barModel->slots[0].timing.open && barModel->slots[0].timing.progress > .69f && barModel->slots[0].timing.progress < .71f, "real HUD presenter projects timing marker and both windows");
		AbilityBarView view{ app.GetEngineContext(), barModel }; view.ResolveLayoutTree({ { 0.f, 0.f }, { 1280.f, 720.f } }, true); view.NativeTick(0.f); view.ResolveLayoutTree({ { 0.f, 0.f }, { 1280.f, 720.f } }, true);
		const auto column = view.GetColumn(0); auto canvas = std::find_if(column->GetChildren().begin(), column->GetChildren().end(), [](const auto& child) { return std::dynamic_pointer_cast<HoloCanvas>(child) != nullptr; });
		report.Check(canvas != column->GetChildren().end() && (*canvas)->GetVisibility() && (*canvas)->GetResolvedRect().position.y < 720.f, "Timing bar is visible at the bottom of the viewport");
		auto& window = app.GetRenderWindow(); const auto oldView = window.getView(); window.setSize({ 1280u, 720u }); window.setView(sf::View{ sf::FloatRect{ { 0.f, 0.f }, { 1280.f, 720.f } } }); window.clear(sf::Color{ 8, 12, 22 }); view.NativeDraw(window);
		sf::Texture texture; report.Check(texture.resize(window.getSize()), "Timing HUD screenshot texture"); texture.update(window); report.Check(texture.copyToImage().saveToFile(std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "outputs/ability-timing-hud.png"), "save rendered Timing HUD for visual review"); window.display(); window.setSize({ 64u, 64u }); window.setView(oldView);
		report.Check(player.TryUninstallModule(rShell.instanceId) && player.TryInstallModule(elementId, rShell.origin, 0) == ModulePlacementFailure::None, "Player installs stored Element Core");
		ship->Destroy(); level->TickInternal(0.f); const auto respawn = player.SpawnSpaceShip(level.get()).lock(); level->TickInternal(0.f);
		report.Check(respawn && player.GetModuleLoadout().FindPlacement(elementId)->element == stored && player.GetModuleLoadout().FindPlacement(elementId)->abilitySlot == R && respawn->GetAbilitySystemComponent().GetActivationPolicy().GetDamageType(R) == stored, "real Player respawn restores the same stored element and slot policy");
	}
}

int main(int argc, char** argv)
{
	vega::EngineContext context; vega::EngineContext::Scope scope{ context }; vega::InstallVegaEngineAssets(); if (argc != 2) return 2;
	Report report; report.artifact["test"] = "AbilityModulesE2E";
	try
	{
		context.Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Ability module content registration failed" };
		std::string title = "Ability Modules E2E"; Application app{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None }; app.GetRenderWindow().setVisible(false);
		Content(report); TimingChecks(report, app); AutocastChecks(report, app); ElementChecks(report, app); FortuneChecks(report, app); PlacementAndHud(report, app); report.artifact["passed"] = true;
	}
	catch (const std::exception& error) { report.artifact["error"] = error.what(); std::cerr << error.what() << '\n'; }
	WriteArtifact(argv[1], report.artifact); return report.artifact.value("passed", false) ? 0 : 1;
}
