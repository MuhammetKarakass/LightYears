#include "framework/EngineContext.h"

#include <algorithm>
#include "gameConfigs/CollisionLayers.h"
#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP6E2E.h"
#include "gameplay/module/rules/EpicUtilityMovementRuntime.h"
#include "gameplay/ability/frozenThrong/FrozenThrongHuskActor.h"
#include "gameplay/ability/frozenThrong/FrozenThrongContracts.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/projectile/ProjectilePhaseSlip.h"
#include "player/Player.h"
#include "player/PlayerMovementComponent.h"
#include "gameConfigs/SimulationTimeDomains.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/directionalBarrier/DirectionalBarrierContracts.h"
#include "gameplay/ability/voidGate/VoidGatePortalActor.h"
#include "gameplay/effects/content/directionalBarrier/DirectionalBarrierEffectBehavior.h"
#include "framework/MathUtility.h"

#include <cmath>

namespace
{
	using namespace uncommon_p6_e2e;
	const char* const kCryoBola = "Ability.Utility.CryoBola.Basic";
	class PhaseWall final : public Actor
	{
	public:
		explicit PhaseWall(World* world) : Actor{ world }
		{
			SetPhysicsBodyType(PhysicsBodyType::Static);
			SetCollisionLayer(GameCollision::Environment);
			SetCollisionMask(GameCollision::EnemyBullet);
		}
		sf::Vector2f GetPhysicsCollisionBoxHalfExtents() const override { return { 5.f, 80.f }; }
	};

	ModuleEpicUtilityMovementRuntime& Epic(Fixture& f) { return f.ship->GetModuleApplier().GetEpicUtilityMovementRuntime(); }

	void PhaseChecks(Report& report, Application& application)
	{
		for (const int count : { 0, 1, 12 })
		{
			Fixture f{ report, application };
			const auto id = f.Install("Module.Movement.PhaseSlip.Epic");
			PlayerMovementComponent::SetDevAutoPlay(true);
			f.ship->Tick(.01f);
			PlayerMovementComponent::SetDevAutoPlay(false);
			report.Check(Epic(f).IsPhaseActive(), "Afterburner starts the Phase Slip window");
			report.Check(!f.ship->GetAbilitySystemComponent().IsModuleSystemEnabled(GameplayTags::Module::System::PrimaryWeapon), "phase locks Primary");
			report.Check(!f.ship->GetAbilitySystemComponent().IsModuleSystemEnabled(GameplayTags::Module::System::AbilitySlot1), "phase locks loadout abilities");
			f.Equal("phase cooldown remains six seconds", 6.f, Epic(f).GetPhaseCooldown());
			const auto enemy = Enemy(f, 600.f);
			f.ship->GetShieldComponent().ChangeShield(-10000.f);
			for (const auto delivery : { DamageDeliveryType::Beam, DamageDeliveryType::Area, DamageDeliveryType::Contact })
				report.Check(Hit(*f.ship, enemy.get(), 1.f, Plain(), {}, {}, delivery) > 0.f, "beam area and contact damage still hit during phase");
			const auto* weapon = content::WeaponContentCatalog::FindById("Weapon.Projectile.EnemyVanguardPulse.Basic");
			report.Check(weapon != nullptr, "phase uses a shipped hostile projectile");
			const auto origin = f.ship->GetActorLocation();
			const auto friendly = f.level->SpawnActor<PrimaryWeaponProjectileActor>(f.ship.get(), weapon->presentationDefinition, weapon->attributes).lock();
			report.Check(!TryPassPhaseSlipContact(*friendly, f.ship.get()) && Epic(f).GetPhaseProjectileCount() == 0, "friendly projectiles do not charge Phase Slip");
			friendly->Destroy();
			if (count == 0)
			{
				const auto wall = f.level->SpawnActor<PhaseWall>().lock();
				wall->SetActorLocation(origin - sf::Vector2f{ 50.f, 0.f });
				f.level->TickInternal(0.f);
				const auto blocked = f.level->SpawnActor<PrimaryWeaponProjectileActor>(enemy.get(), weapon->presentationDefinition, weapon->attributes).lock();
				blocked->SetActorLocation(origin - sf::Vector2f{ 100.f, 0.f });
				blocked->SetLaunchVelocity({ 1000.f, 0.f });
				blocked->Tick(.2f);
				report.Check(blocked->GetIsPendingDestroy() && Epic(f).GetPhaseProjectileCount() == 0, "a projectile blocked by an earlier wall never counts as passing the ship");
				wall->Destroy(); f.level->TickInternal(0.f);
			}
			List<shared_ptr<PrimaryWeaponProjectileActor>> passed;
			for (int index = 0; index < count; ++index)
			{
				const auto projectile = f.level->SpawnActor<PrimaryWeaponProjectileActor>(enemy.get(), weapon->presentationDefinition, weapon->attributes).lock();
				projectile->SetDamagePayload(Plain());
				projectile->SetActorLocation(origin - sf::Vector2f{ 100.f, 0.f });
				projectile->SetLaunchVelocity({ 1000.f, 0.f });
				const float health = f.ship->GetHealthComponent().GetHealth();
				projectile->OnActorBeginOverlap(f.ship.get());
				projectile->OnActorBeginOverlap(f.ship.get());
				projectile->Tick(.2f);
				report.Check(!projectile->GetIsPendingDestroy() && projectile->GetActorLocation().x > origin.x, "hostile projectile passes and keeps travelling without consumption");
				f.Equal("phased projectile deals no damage", health, f.ship->GetHealthComponent().GetHealth());
				passed.push_back(projectile);
			}
			f.Equal("duplicate contact counts each projectile only once without a stack cap", static_cast<float>(count), static_cast<float>(Epic(f).GetPhaseProjectileCount()));
			f.ship->GetModuleApplier().Tick(.34f);
			report.Check(Epic(f).IsPhaseActive(), "phase is still open before 0.35 seconds");
			f.ship->GetModuleApplier().Tick(.011f);
			report.Check(!Epic(f).IsPhaseActive() && Epic(f).IsPhaseArmed(), "phase ends after 0.35 seconds and arms one Primary hit");
			const auto rule = f.ship->GetModuleApplier().GetResolvedEffects().epicRules.phaseSlips.front();
			const auto near = Enemy(f, 900.f);
			const auto edge = Enemy(f, 900.f);
			near->SetActorLocation(enemy->GetActorLocation() + sf::Vector2f{ rule.explosionRadius - 2.f, 0.f });
			edge->SetActorLocation(enemy->GetActorLocation() + sf::Vector2f{ rule.explosionRadius + 15.f * count - 1.f, 0.f });
			const auto outside = Enemy(f, 2000.f);
			outside->SetActorLocation(enemy->GetActorLocation() + sf::Vector2f{ rule.explosionRadius + 15.f * count + 10.f, 0.f });
			Hit(*enemy, f.ship.get(), 10.f, Plain(), {}, { GameplayTags::Ability::Primary });
			f.ship->GetModuleApplier().Tick(0.f);
			const float explosion = 10.f * rule.primaryDamageMultiplier * (1.f + .25f * count);
			f.Equal("passed projectiles add 25 percent explosion damage each", 1000.f - explosion, near->GetHealthComponent().GetHealth());
			f.Equal("passed projectiles add fifteen radius each", 1000.f - explosion, edge->GetHealthComponent().GetHealth());
			f.Equal("outside the grown radius remains unharmed", 1000.f, outside->GetHealthComponent().GetHealth());
			report.Check(!Epic(f).IsPhaseArmed(), "the first Primary hit consumes the explosion");
			Hit(*enemy, f.ship.get(), 10.f, Plain(), {}, { GameplayTags::Ability::Primary });
			f.ship->GetModuleApplier().Tick(0.f);
			f.Equal("a second Primary hit does not explode again", 1000.f - explosion, near->GetHealthComponent().GetHealth());
			if (!passed.empty())
			{
				passed.front()->OnActorBeginOverlap(f.ship.get());
				report.Check(passed.front()->GetIsPendingDestroy(), "the same projectile can hit normally after the phase");
			}
			f.Remove(id);
		}
	}

	void PhaseAbilityProjectileChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		f.Install("Module.Movement.PhaseSlip.Epic");
		PlayerMovementComponent::SetDevAutoPlay(true);
		f.ship->Tick(.01f);
		PlayerMovementComponent::SetDevAutoPlay(false);
		const auto enemy = Enemy(f, 600.f);
		const auto* definition = AbilityData::FindAbilityActorDefinition(AbilityData::FrozenThrong::Actor::Husk::BasicDefinitionId);
		report.Check(definition != nullptr, "phase ability projectile uses its shipped actor definition");
		const auto husk = AbilityActorRegistry::Spawn({ *enemy, *definition, definition->attributes }).lock();
		report.Check(husk && Epic(f).IsPhaseActive(), "hostile ability projectile spawns during the phase");
		husk->ConfigureFromAttributes(definition->attributes);
		husk->ConfigureCollisionFromOwner();
		husk->SetActorLocation(f.ship->GetActorLocation());
		husk->OnActorBeginOverlap(f.ship.get());
		report.Check(!husk->GetIsPendingDestroy() && husk->GetVelocity() != sf::Vector2f{} && Epic(f).GetPhaseProjectileCount() == 1, "ability projectile contact also passes without exploding or stopping");
		f.ship->GetModuleApplier().Tick(.351f);
		husk->OnActorBeginOverlap(f.ship.get());
		report.Check(husk->GetVelocity() == sf::Vector2f{}, "ability projectile contact resumes its normal explosion after the phase");
	}

	void CascadeChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		const auto id = f.Install("Module.Utility.CascadeReactor.Epic", 0, 1);
		CastOffense(f);
		f.Equal("the first use starts the chain without a bonus", 0.f, static_cast<float>(Epic(f).GetCascadeStep(id)));
		CastMovement(f);
		f.Equal("a different family takes step one", 1.f, static_cast<float>(Epic(f).GetCascadeStep(id)));
		CastOffense(f);
		f.Equal("a different family again takes step two", 2.f, static_cast<float>(Epic(f).GetCascadeStep(id)));
		CastOffense(f);
		f.Equal("repeating the same family resets the chain", 0.f, static_cast<float>(Epic(f).GetCascadeStep(id)));
		CastMovement(f);
		f.Equal("the chain restarts from the reset family", 1.f, static_cast<float>(Epic(f).GetCascadeStep(id)));
		f.ship->GetModuleApplier().Tick(5.1f);
		f.Equal("a lapsed five second window resets the chain", 0.f, static_cast<float>(Epic(f).GetCascadeStep(id)));
		f.Remove(id);
		report.Check(Epic(f).GetCascadeStep(id) == 0, "uninstall removes the chain state");
	}

	GameAbility& CascadeAbility(Fixture& f, const char* id, sas::AbilitySlot slot)
	{
		auto& asc = f.ship->GetAbilitySystemComponent();
		if (const auto* old = asc.GetAbility(slot)) f.report.Check(asc.RemoveAbility(old->GetHandle()), "Cascade fixture replaces only the requested ability slot");
		auto definition = *content::AbilityContentCatalog::FindById(id); definition.slot = slot;
		f.report.Check(asc.GrantAbility(definition).IsValid(), "Cascade fixture grants shipped ability content");
		return *asc.GetAbilityById(id);
	}

	void StartCascadeAbility(Fixture& f, GameAbility& ability)
	{
		if (ability.IsActive()) ability.Cancel(sas::AbilityEndReason::Completed);
		ability.Tick(100.f);
		f.report.Check(ability.TryActivate(), "Cascade quantity scenario activates through the real ASC");
	}

	float CascadeStepBonus(Fixture& f, std::uint32_t instance, int level = 1)
	{
		const int effectiveLevel = level + ModuleResolver::FindZoneLevelBonus(f.ship->GetModuleApplier().GetResolvedEffects(), instance);
		const float expected = .20f + .02f * static_cast<float>(effectiveLevel - 1);
		f.Equal("Cascade coefficient follows placement plus zone level", expected, f.ship->GetModuleApplier().GetResolvedEffects().epicRules.cascades.front().stepBonus);
		return expected;
	}

	void CascadeTimeSlipChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		auto& ability = CascadeAbility(f, "Ability.Defense.TimeSlip.Basic", sas::AbilitySlot::Ability1);
		StartCascadeAbility(f, ability);
		const float duration = ability.GetActiveDuration();
		f.Equal("unpowered Time Slip preserves hostile time", .35f, f.level->GetSimulationTimeScale(GameTimeDomains::HostileGameplay));
		ability.Cancel(sas::AbilityEndReason::Completed);
		const auto id = f.Install("Module.Utility.CascadeReactor.Epic");
		const float multiplier = 1.f + CascadeStepBonus(f, id);
		CastOffense(f); StartCascadeAbility(f, ability);
		f.Equal("Time Slip captures Cascade before its use advances the chain", multiplier, ability.GetDefinition().mainEffectMultiplier);
		f.Equal("Cascade divides hostile gameplay time", .35f / multiplier, f.level->GetSimulationTimeScale(GameTimeDomains::HostileGameplay));
		f.Equal("Cascade divides all projectile gameplay time", .35f / multiplier, f.level->GetSimulationTimeScale(GameTimeDomains::ProjectileGameplay));
		f.Equal("Cascade preserves Primary fire rate penalty", .35f, f.ship->GetPrimaryWeaponFireRateMultiplier());
		f.Equal("Cascade preserves Time Slip duration", duration, ability.GetActiveDuration());
		CastOffense(f); f.ship->GetModuleApplier().Tick(5.01f); f.Remove(id);
		f.Equal("active Time Slip keeps its captured time after chain expiry and removal", .35f / multiplier, f.level->GetSimulationTimeScale(GameTimeDomains::HostileGameplay));
		f.Equal("active projectile time also keeps its captured multiplier", .35f / multiplier, f.level->GetSimulationTimeScale(GameTimeDomains::ProjectileGameplay));
		ability.Tick(duration + .01f);
		report.Check(!ability.IsActive(), "Time Slip ends at its unchanged duration");
		f.Equal("Time Slip ending restores hostile time", 1.f, f.level->GetSimulationTimeScale(GameTimeDomains::HostileGameplay));
		f.Equal("Time Slip ending restores projectile time", 1.f, f.level->GetSimulationTimeScale(GameTimeDomains::ProjectileGameplay));
		f.Equal("Time Slip ending restores Primary fire rate", 1.f, f.ship->GetPrimaryWeaponFireRateMultiplier());
	}

	void CascadeBarrierChecks(Report& report, Application& application)
	{
		for (const int step : { 0, 1, 3 })
		{
			Fixture f{ report, application };
			auto& ability = CascadeAbility(f, "Ability.Defense.DirectionalBarrier.Basic", sas::AbilitySlot::Ability1);
			StartCascadeAbility(f, ability); const float duration = ability.GetActiveDuration(); ability.Cancel(sas::AbilityEndReason::Completed);
			const auto id = f.Install("Module.Utility.CascadeReactor.Epic");
			const float multiplier = 1.f + step * CascadeStepBonus(f, id);
			if (step > 0) CastOffense(f);
			if (step == 3) { CastMovement(f); CastOffense(f); }
			StartCascadeAbility(f, ability);
			const float halfAngle = std::min(60.f * multiplier, 90.f);
			const auto* effect = Effect(*f.ship, AbilityData::DirectionalBarrier::Effect::ActiveEffectId);
			report.Check(effect != nullptr, "Barrier owns a real active effect snapshot");
			f.Equal("Barrier captures its Cascade multiplier", multiplier, ability.GetDefinition().mainEffectMultiplier);
			f.Equal("Barrier arc is 120 degrees times Cascade capped at 180", halfAngle * 2.f, 2.f * sas::FindAttributeValue(effect->spec.attributes, AbilityData::DirectionalBarrier::Attribute::HalfAngleDegrees, 0.f));
			f.Equal("Cascade preserves Barrier duration", duration, ability.GetActiveDuration());
			if (step == 3) report.Check(multiplier > 1.5f && halfAngle == 90.f, "third step exercises the actual 180 degree cap");
			f.Remove(id);
			const auto enemy = Enemy(f, 600.f);
			const auto* weapon = content::WeaponContentCatalog::FindById("Weapon.Projectile.EnemyVanguardPulse.Basic");
			report.Check(weapon != nullptr, "Barrier angle probes use shipped hostile projectiles");
			f.ship->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Armor, 0.f);
			f.ship->GetShieldComponent().ChangeShield(-10000.f);
			f.level->TickInternal(0.f);
			for (const float angle : { -(halfAngle + .5f), -(halfAngle - .5f), halfAngle - .5f, halfAngle + .5f })
			{
				const float radians = angle * .01745329251994329577f;
				const auto forward = f.ship->GetActorForwardDirection();
				const sf::Vector2f direction{ forward.x * std::cos(radians) - forward.y * std::sin(radians), forward.x * std::sin(radians) + forward.y * std::cos(radians) };
				const auto projectile = f.level->SpawnActor<PrimaryWeaponProjectileActor>(enemy.get(), weapon->presentationDefinition, weapon->attributes).lock();
				projectile->SetActorLocation(f.ship->GetActorLocation() + direction * 100.f); projectile->SetLaunchVelocity(-direction * 1000.f);
				const bool blocked = std::abs(angle) < halfAngle;
				report.Check(DirectionalBarrierEffectBehavior::TryInterceptProjectile(*projectile, f.ship->GetActorLocation() + direction * 200.f) == blocked, "swept Barrier interception respects both captured arc edges after module removal", { { "step", step }, { "angle", angle }, { "blocked", blocked } });
				const float health = f.ship->GetHealthComponent().GetHealth();
				ApplyCombatDamage(*f.ship, 1.f, enemy.get(), {}, Plain(), {}, {}, DamageDeliveryType::Projectile, projectile.get());
				f.Equal("damage pipeline respects the same captured Barrier edges", blocked ? 0.f : 1.f, health - f.ship->GetHealthComponent().GetHealth());
				projectile->Destroy();
			}
			ability.Tick(duration + .01f);
			report.Check(!ability.IsActive() && !Effect(*f.ship, AbilityData::DirectionalBarrier::Effect::ActiveEffectId), "Barrier expires and removes its arc snapshot at the unchanged duration");
		}
	}

	void CascadeVectorSyncChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		f.ship->GetMovementComponent().SetMovementMode(ShipMovementMode::ThrustDrift);
		auto& ability = CascadeAbility(f, "Ability.Movement.VectorSync.Basic", sas::AbilitySlot::Ability2);
		const float baseline = f.ship->GetMovementSpeedMultiplier();
		StartCascadeAbility(f, ability); const float duration = ability.GetActiveDuration();
		f.Equal("unpowered Vector Sync keeps the existing movement speed", baseline, f.ship->GetMovementSpeedMultiplier());
		report.Check(f.ship->GetMovementComponent().HasDirectionalThrustSync(), "Vector Sync keeps directional thrust equalization");
		ability.Cancel(sas::AbilityEndReason::Completed);
		const auto id = f.Install("Module.Utility.CascadeReactor.Epic"); const float multiplier = 1.f + CascadeStepBonus(f, id);
		CastOffense(f); StartCascadeAbility(f, ability);
		f.Equal("Vector Sync captures Cascade", multiplier, ability.GetDefinition().mainEffectMultiplier);
		f.Equal("active Vector Sync multiplies movement speed", baseline * multiplier, f.ship->GetMovementSpeedMultiplier());
		f.Equal("Cascade preserves Vector Sync duration", duration, ability.GetActiveDuration());
		const float expectedCap = f.ship->GetMovementComponent().ResolveCurrentSpeedCap(baseline * multiplier);
		f.ship->SetVelocity({ 100000.f, 0.f }); f.ship->SpaceShip::Tick(.001f);
		f.Equal("real movement clamps at the Cascade-powered speed", expectedCap, GetVectorLength(f.ship->GetVelocity()));
		CastOffense(f); f.ship->GetModuleApplier().Tick(5.01f); f.Remove(id);
		f.Equal("active Vector Sync retains its snapshot after the chain changes", baseline * multiplier, f.ship->GetMovementSpeedMultiplier());
		ability.Tick(duration + .01f);
		report.Check(!ability.IsActive() && !f.ship->GetMovementComponent().HasDirectionalThrustSync(), "Vector Sync expires and releases directional thrust sync");
		f.Equal("Vector Sync expiration removes the captured speed bonus", baseline, f.ship->GetMovementSpeedMultiplier());
	}

	void CascadeVoidGateChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		auto& ability = CascadeAbility(f, "Ability.Utility.VoidGate.Basic", sas::AbilitySlot::Ability1);
		StartCascadeAbility(f, ability); const float duration = ability.GetActiveDuration();
		report.Check(Actors<VoidGatePortalActor>(f).size() == 1, "Void Gate creates its normal portal preview");
		const float radius = Actors<VoidGatePortalActor>(f).front()->GetPortalRadius();
		AbilityExecutionContext execution{ &f.ship->GetAbilitySystemComponent(), &ability.GetDefinition(), nullptr, &ability };
		const auto baseline = AbilityActionAttributeResolver::ResolveAbilityAttributes(execution);
		ability.Cancel(sas::AbilityEndReason::Completed);
		const auto id = f.Install("Module.Utility.CascadeReactor.Epic"); const float bonus = CascadeStepBonus(f, id);
		CastOffense(f); StartCascadeAbility(f, ability);
		f.Equal("Void Gate participates as the Utility step", 1.f, static_cast<float>(Epic(f).GetCascadeStep(id)));
		f.Equal("Void Gate receives the existing captured multiplier", 1.f + bonus, ability.GetDefinition().mainEffectMultiplier);
		f.Equal("Cascade does not grow Void Gate duration", duration, ability.GetActiveDuration());
		report.Check(Actors<VoidGatePortalActor>(f).size() == 1, "powered Void Gate still creates one preview");
		f.Equal("Cascade does not grow the actual portal radius", radius, Actors<VoidGatePortalActor>(f).front()->GetPortalRadius());
		const auto powered = AbilityActionAttributeResolver::ResolveAbilityAttributes(execution);
		report.Check(powered.size() == baseline.size(), "Void Gate keeps its authored runtime channels");
		for (const auto& attribute : baseline) report.CheckValue("Cascade leaves every Void Gate runtime value unchanged", attribute.currentValue, sas::FindAttributeValue(powered, attribute.id, -1.f), std::string{ attribute.id.GetName() });
		CastOffense(f);
		f.Equal("Offense continues from Void Gate's Utility step", 2.f, static_cast<float>(Epic(f).GetCascadeStep(id)));
		StartCascadeAbility(f, ability); StartCascadeAbility(f, ability);
		f.Equal("repeated Void Gate resets as the same Utility family", 0.f, static_cast<float>(Epic(f).GetCascadeStep(id)));
		ability.Cancel(sas::AbilityEndReason::Completed);
	}

	void FieldMasteryChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		const auto id = f.Install("Module.Utility.FieldMastery.Epic", 0, 1);
		const List<GameplayTag> utility{ GameplayTags::Ability::Utility };
		f.Equal("Utility abilities cannot be upgraded with Scrap while installed", 1.f, Epic(f).IsScrapUpgradeLocked(utility) ? 1.f : 0.f);
		// Cryo Bola is a shipped Utility ability (default loadout Ability1) and the real source of Utility uses here.
		auto& abilities = f.ship->GetAbilitySystemComponent();
		if (!abilities.GetAbilityById(kCryoBola))
		{
			if (auto* current = abilities.GetAbility(sas::AbilitySlot::Ability1)) report.Check(abilities.RemoveAbility(current->GetHandle()), "Field Mastery test replaces only the fixture's Q ability");
			const auto* authored = content::AbilityContentCatalog::FindById(kCryoBola);
			report.Check(authored != nullptr, "Field Mastery test uses shipped Cryo Bola content");
			auto definition = *authored; definition.slot = sas::AbilitySlot::Ability1;
			report.Check(abilities.GrantAbility(definition).IsValid(), "Field Mastery test grants the shipped Cryo Bola Utility ability");
		}
		const auto* ability = abilities.GetAbilityById(kCryoBola);
		report.Check(ability != nullptr, "Field Mastery test resolves the Cryo Bola Utility ability on the ship");
		report.Check(std::any_of(ability->GetDefinition().abilityTags.begin(), ability->GetDefinition().abilityTags.end(), [](const GameplayTag& tag) { return tag.MatchesTag(GameplayTags::Ability::Utility); }), "Field Mastery test ability carries the Utility tag");
		f.Equal("the granted Utility ability starts at run level one", 1.f, static_cast<float>(ability->GetLevel()));
		for (int use = 0; use < 14; ++use) Epic(f).OnAbilityUse(kCryoBola, utility);
		f.Equal("uses accumulate per ability", 14.f, Epic(f).GetUseCount(kCryoBola));
		f.ship->GetModuleApplier().Tick(0.01f);
		f.Equal("fourteen uses do not grant a run level yet", 1.f, static_cast<float>(ability->GetLevel()));
		Epic(f).OnAbilityUse(kCryoBola, utility);
		f.ship->GetModuleApplier().Tick(0.01f);
		f.Equal("the fifteenth use grants one run level", 2.f, static_cast<float>(ability->GetLevel()));
		f.Equal("the threshold is spent", 0.f, Epic(f).GetUseCount(kCryoBola));
		f.Remove(id);
		report.Check(!Epic(f).IsScrapUpgradeLocked(utility), "uninstall lifts the Scrap lock");
	}

	void FieldRunChecks(Report& report, Application& application)
	{
		auto level = std::make_shared<HeadlessModuleLevel>(&application);
		level->BeginPlayInternal();
		Player player;
		player.AddLifeCount(3);
		auto ship = player.SpawnSpaceShip(level.get()).lock();
		report.Check(ship != nullptr, "Field run state spawns through Player");
		level->TickInternal(0.f);
		const auto rules = player.BuildModuleRules();
		const auto id = player.AddModuleToInventory("Module.Utility.FieldMastery.Epic", 0, ModuleRarity::Epic);
		report.Check(rules.has_value() && id != 0, "Player owns a real Field Mastery inventory item");
		ModulePlacement placement = *player.GetModuleLoadout().FindInventoryItem(id);
		bool installed = false;
		for (int rotation = 0; rotation < 4 && !installed; ++rotation)
			for (int y = 0; y < rules->schematic.height && !installed; ++y)
				for (int x = 0; x < rules->schematic.width && !installed; ++x)
				{
					placement.origin = { x, y }; placement.rotationQuarterTurns = rotation;
					if (ModulePlacementRules::CanPlace(*rules, player.GetModuleLoadout(), placement) != ModulePlacementFailure::None) continue;
					installed = player.TryInstallModule(id, placement.origin, rotation) == ModulePlacementFailure::None;
				}
		report.Check(installed, "Player installs Field Mastery");
		const auto grantUtility = [&report](PlayerSpaceShip& target)
		{
			auto& abilities = target.GetAbilitySystemComponent();
			if (abilities.GetAbilityById(kCryoBola)) return;
			if (const auto* current = abilities.GetAbility(sas::AbilitySlot::Ability1)) report.Check(abilities.RemoveAbility(current->GetHandle()), "run fixture replaces Q");
			auto definition = *content::AbilityContentCatalog::FindById(kCryoBola); definition.slot = sas::AbilitySlot::Ability1;
			report.Check(abilities.GrantAbility(definition).IsValid(), "run fixture grants shipped Utility");
		};
		grantUtility(*ship);
		const List<GameplayTag> utility{ GameplayTags::Ability::Utility };
		const auto runtime = [](PlayerSpaceShip& target) -> ModuleEpicUtilityMovementRuntime& { return target.GetModuleApplier().GetEpicUtilityMovementRuntime(); };
		for (int use = 0; use < 9; ++use) runtime(*ship).OnAbilityUse(kCryoBola, utility);
		report.Check(player.TryUninstallModule(id), "Field Mastery uninstalls through Player");
		for (int use = 0; use < 20; ++use) runtime(*ship).OnAbilityUse(kCryoBola, utility);
		report.Check(runtime(*ship).GetUseCount(kCryoBola) == 9.f, "uninstalled counters freeze");
		ship->Destroy(); level->TickInternal(0.f);
		ship = player.SpawnSpaceShip(level.get()).lock(); level->TickInternal(0.f);
		report.Check(ship && runtime(*ship).GetUseCount(kCryoBola) == 9.f, "death and respawn preserve frozen uses at Player level");
		grantUtility(*ship);
		report.Check(player.TryInstallModule(id, placement.origin, placement.rotationQuarterTurns) == ModulePlacementFailure::None, "same Field Mastery reinstalls");
		const int threshold = ship->GetModuleApplier().GetResolvedEffects().epicRules.fieldMasteries.front().usesPerLevelUp;
		for (int use = 9; use < threshold; ++use) runtime(*ship).OnAbilityUse(kCryoBola, utility);
		ship->GetModuleApplier().Tick(0.f);
		report.Check(ship->GetAbilitySystemComponent().GetAbilityById(kCryoBola)->GetLevel() == 2 && runtime(*ship).GetUseCount(kCryoBola) == 0.f, "reinstalled module resumes at nine uses and spends its zone-resolved threshold");
		for (int use = 0; use < 4; ++use) runtime(*ship).OnAbilityUse(kCryoBola, utility);
		ship->Destroy(); level->TickInternal(0.f);
		ship = player.SpawnSpaceShip(level.get()).lock(); level->TickInternal(0.f);
		report.Check(ship && runtime(*ship).GetUseCount(kCryoBola) == 4.f, "installed Field Mastery preserves partial uses through respawn");
		const auto* ability = ship->GetAbilitySystemComponent().GetAbilityById(kCryoBola);
		report.Check(ability && ability->GetLevel() == 2, "earned Utility levels remain restored by Player");
		ship->Destroy(); level->TickInternal(0.f);
		while (player.GetLifeCount() > 0)
		{
			ship = player.SpawnSpaceShip(level.get()).lock(); level->TickInternal(0.f);
			report.Check(ship && runtime(*ship).GetUseCount(kCryoBola) == 4.f, "remaining lives retain the same run counters");
			ship->Destroy(); level->TickInternal(0.f);
		}
		report.Check(player.SpawnSpaceShip(level.get()).expired(), "exhausted lives finish the run");
		player.AddLifeCount(1);
		ship = player.SpawnSpaceShip(level.get()).lock(); level->TickInternal(0.f);
		report.Check(ship && runtime(*ship).GetUseCount(kCryoBola) == 0.f, "a new run starts with fresh Field Mastery counters");
	}

	void AssaultChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		const auto id = f.Install("Module.Movement.AssaultVector.Epic", 0, 1);
		const auto enemy = Enemy(f, 500.f);
		enemy->SetActorLocation(f.ship->GetActorLocation() + sf::Vector2f{ 500.f, 0.f });
		f.ship->GetModuleApplier().Tick(0.01f);
		Move(f, 100.f, .25f);
		f.Equal("moving toward the nearest enemy adds the Level 1 damage bonus", 0.15f, Epic(f).GetAssaultDamageBonus());
		f.Equal("moving toward the enemy adds no Armor", 0.f, Epic(f).GetAssaultArmorBonus());
		Move(f, -100.f, .25f);
		f.Equal("moving away from the enemy clears the damage bonus", 0.f, Epic(f).GetAssaultDamageBonus());
		f.Equal("moving away from the enemy adds the Level 1 Armor bonus", 20.f, Epic(f).GetAssaultArmorBonus());
		for (float x : { 10.f, 0.f, -10.f })
		{
			const sf::Vector2f velocity{ x, 150.f };
			enemy->SetActorLocation(f.ship->GetActorLocation() + velocity * .1f + sf::Vector2f{ 500.f, 0.f });
			const auto source = f.ship->GetUniqueID();
			report.Check(movement::MovementInfluenceService::SetForcedMovement(*f.ship, { source, velocity, 1 }), "Assault direction test moves through the real component");
			f.ship->GetMovementComponent().Tick(.1f); f.ship->GetModuleApplier().Tick(.1f);
			movement::MovementInfluenceService::RemoveSource(*f.ship, source);
			f.Equal("the entire forward half-plane including perpendicular grants damage", x >= 0.f ? .15f : 0.f, Epic(f).GetAssaultDamageBonus());
			f.Equal("only the reverse half-plane grants Armor", x < 0.f ? 20.f : 0.f, Epic(f).GetAssaultArmorBonus());
		}
		Move(f, 0.f, .25f);
		f.Equal("standing still gives neither bonus (damage)", 0.f, Epic(f).GetAssaultDamageBonus());
		f.Equal("standing still gives neither bonus (Armor)", 0.f, Epic(f).GetAssaultArmorBonus());
		f.Remove(id);
		f.Equal("uninstall clears the Armor bonus", 0.f, Epic(f).GetAssaultArmorBonus());
	}

	void SideChecks(Report& report, Application& application)
	{
		Fixture side{ report, application };
		const auto id = side.Install("Module.Utility.FieldMastery.Epic", 1, 1, "Module.SideFeature.FieldMastery.KnowledgeTransfer");
		const auto& fields = side.ship->GetModuleApplier().GetResolvedEffects().epicRules.fieldMasteries;
		const bool transfer = std::any_of(fields.begin(), fields.end(), [](const ResolvedModuleFieldMasteryRule& rule) { return rule.transferUses == 2.f; });
		report.Check(transfer, "Knowledge Transfer adds two transfer uses at the three-cell size");
		side.Remove(id);

		Fixture small{ report, application };
		small.Install("Module.Utility.FieldMastery.Epic", 0, 1, "Module.SideFeature.FieldMastery.KnowledgeTransfer");
		const auto& smallFields = small.ship->GetModuleApplier().GetResolvedEffects().epicRules.fieldMasteries;
		report.Check(std::none_of(smallFields.begin(), smallFields.end(), [](const ResolvedModuleFieldMasteryRule& rule) { return rule.transferUses > 0.f; }), "two-cell Field Mastery keeps its side locked");
	}

	void ContentChecks(Report& report)
	{
		const auto* cascade = content::ModuleContentCatalog::FindById("Module.Utility.CascadeReactor.Epic");
		const auto* field = content::ModuleContentCatalog::FindById("Module.Utility.FieldMastery.Epic");
		const auto* assault = content::ModuleContentCatalog::FindById("Module.Movement.AssaultVector.Epic");
		const auto* phase = content::ModuleContentCatalog::FindById("Module.Movement.PhaseSlip.Epic");
		report.Check(cascade && cascade->rarity == vega::ModuleRarity::Epic && cascade->sideFeatures.empty(), "Cascade Reactor is a sideless Epic Utility");
		report.Check(field && field->rarity == vega::ModuleRarity::Epic && field->sizeLadderCellCounts == List<int>{ 2, 3 } && field->sideFeatures.size() == 2, "Field Mastery is a 2/3 Epic ladder with two sides");
		report.Check(assault && assault->rarity == vega::ModuleRarity::Epic && assault->sideFeatures.empty(), "Assault Vector is a sideless Epic Movement");
		report.Check(phase && phase->rarity == vega::ModuleRarity::Epic && phase->sideFeatures.empty(), "Phase Slip is a sideless Epic Movement");
		report.Check(content::ModuleSideFeatureCatalog::FindById("Module.SideFeature.FieldMastery.KnowledgeTransfer") != nullptr, "Knowledge Transfer side is registered");
	}
}

int main(int argc, char** argv)
{
	// Setup below runs before any Application exists; applications adopt this binding.
	vega::EngineContext testContext;
	vega::EngineContext::Scope testScope{ testContext };
	vega::InstallVegaEngineAssets();
	if (argc != 2) return 2;
	Report report;
	try
	{
		vega::EngineContext::Current().Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Epic Utility/Movement content registration failed" };
		std::string title = "Epic Utility Movement E2E"; Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None }; application.GetRenderWindow().setVisible(false);
		ContentChecks(report); PhaseChecks(report, application); PhaseAbilityProjectileChecks(report, application); CascadeChecks(report, application); CascadeTimeSlipChecks(report, application); CascadeBarrierChecks(report, application); CascadeVectorSyncChecks(report, application); CascadeVoidGateChecks(report, application); FieldMasteryChecks(report, application); FieldRunChecks(report, application); AssaultChecks(report, application); SideChecks(report, application); report.artifact["passed"] = true;
	}
	catch (const std::exception& error) { report.artifact["error"] = error.what(); std::cerr << error.what() << '\n'; }
	report.artifact["test"] = "EpicUtilityMovementModulesE2E"; WriteArtifact(argv[1], report.artifact); return report.artifact.value("passed", false) ? 0 : 1;
}
