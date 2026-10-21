#include "framework/EngineContext.h"
#include "gameConfigs/CollisionLayers.h"
#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP4E2E.h"

#include "gameConfigs/combat/WeaponStructs.h"
#include "gameplay/damage/DamageHitGroup.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/content/ModuleContentCatalog.h"
#include "gameplay/content/ModuleLoader.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "player/PlayerSpaceShip.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <variant>

namespace rare_sustained_speed_e2e
{
	using namespace uncommon_p4_e2e;
	using Json = common_module_e2e::Json;

	namespace
	{
		constexpr const char* kSonicId = "Module.Movement.RareSustainedFixture.Rare";
		constexpr const char* kLingerId = "Module.Movement.RareSustainedLingerFixture.Rare";
		// Probe only: a plain PrimaryShotOnly multiplier, to see whether module damage rules reach a live projectile hit.
		constexpr const char* kPrimaryOnlyId = "Module.Movement.RareSustainedPrimaryOnlyFixture.Rare";
		constexpr float kDriveStep = 0.1f;
		// Commanded far above any cap: the ThrustDrift clamp leaves the ship exactly at its live cap.
		constexpr float kFastSpeed = 100000.f;
		constexpr float kProbeDamage = 10.f;
		constexpr float kProbeSpeed = 1100.f;

		// Test-only definitions parsed from fixture JSON. They never enter shipped content.
		vega::List<vega::ModuleDefinition> gFixtureDefinitions;

		const ModuleDefinition* FixtureLookup(const sas::ContentId& id)
		{
			for (const ModuleDefinition& definition : gFixtureDefinitions)
				if (definition.moduleId == id) return &definition;
			return content::ModuleContentCatalog::FindById(id);
		}

		std::filesystem::path WriteJson(const std::filesystem::path& root, const std::string& name, const Json& value)
		{
			std::filesystem::create_directories(root);
			const std::filesystem::path path = root / name;
			std::ofstream output{ path, std::ios::out | std::ios::trunc };
			output << value.dump(2) << '\n';
			return path;
		}

		Json RareModule(const char* id, Json variantFields)
		{
			Json variant{ { "shape", Json::array({ Json::array({ 0, 0 }) }) } };
			for (const auto& field : variantFields.items()) variant[field.key()] = field.value();
			return Json{
				{ "id", id }, { "displayName", "Rare sustained speed fixture" }, { "rarity", "Rare" }, { "levelCount", 3 },
				{ "family", "Ability.Movement" }, { "sizeLadder", Json::array({ 1 }) },
				{ "sizeVariants", Json::array({ std::move(variant) }) }
			};
		}

		// Sonic: hold 2 sec above 90% of cap, Primary speed x2, pierce +1, Primary damage x1.10 / x1.15 / x1.20 by level.
		Json SonicRules()
		{
			return Json::array({ Json{ { "speedFraction", 0.9 }, { "holdSeconds", 2.0 }, { "primarySpeedMultiplier", 2.0 },
				{ "primaryPierceBonus", 1.0 }, { "primaryDamageMultiplier", Json::array({ 1.10, 1.15, 1.20 }) } } });
		}

		// Afterglow: the same state with a scalar damage multiplier and 1.5 sec of linger after the speed drops.
		Json LingerRules()
		{
			return Json::array({ Json{ { "speedFraction", 0.9 }, { "holdSeconds", 2.0 }, { "primarySpeedMultiplier", 2.0 },
				{ "primaryPierceBonus", 1.0 }, { "primaryDamageMultiplier", 1.10 }, { "lingerSeconds", 1.5 } } });
		}

		Json FixtureSet(const Json& shipped, const Json& sonicRules, const Json& lingerRules)
		{
			Json modules = shipped;
			modules["modules"].push_back(RareModule(kSonicId, Json{ { "sustainedSpeedRules", sonicRules } }));
			modules["modules"].push_back(RareModule(kLingerId, Json{ { "sustainedSpeedRules", lingerRules } }));
			modules["modules"].push_back(RareModule(kPrimaryOnlyId, Json{ { "damageRules", Json::array({ Json{ { "condition", "PrimaryShotOnly" }, { "damageMultiplier", 1.5 } } }) } }));
			return modules;
		}

		bool Near(float actual, float expected)
		{
			return std::abs(actual - expected) <= 1e-3f * std::max(1.f, std::abs(expected)) + 1e-3f;
		}

		struct PrimaryValues
		{
			float damage = 0.f;
			float speed = 0.f;
			float pierce = 0.f;
			float outputMultiplier = 1.f;
		};

		sas::GameplayAttributeList Probe()
		{
			return sas::GameplayAttributeList{
				sas::GameplayAttribute{ CommonAttributeIds::Damage, kProbeDamage, 0.f, 100000.f },
				sas::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Speed, kProbeSpeed, 0.f, 100000.f },
				sas::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::PierceCount, 0.f, 0.f, 100.f }
			};
		}

		// Resolves the live PrimaryFire weapon through the same resolver the fire action uses.
		PrimaryValues ResolvePrimary(PlayerSpaceShip& ship)
		{
			GameAbility* primary = ship.GetAbilitySystemComponent().GetAbility(sas::AbilitySlot::PrimaryFire);
			const FireWeaponAction* action = nullptr;
			if (primary)
			{
				for (const AbilityActionSpec& spec : primary->GetDefinition().actions)
				{
					if ((action = std::get_if<FireWeaponAction>(&spec.action))) break;
				}
			}
			if (!primary || !action) throw std::runtime_error{ "Rare sustained speed: no live PrimaryFire weapon" };
			const PrimaryWeaponDefinition weapon = action->weaponDefinition;
			GameAbilityDefinition definition = primary->GetDefinition();
			AbilityExecutionContext context{ &ship.GetAbilitySystemComponent(), &definition, nullptr, nullptr };
			const sas::GameplayAttributeList values = AbilityActionAttributeResolver::ResolveAttributes(context, &weapon, Probe());
			PrimaryValues result;
			result.damage = sas::FindAttributeValue(values, CommonAttributeIds::Damage, -1.f);
			result.speed = sas::FindAttributeValue(values, PrimaryWeaponSchema::Projectile::Delivery::Speed, -1.f);
			result.pierce = sas::FindAttributeValue(values, PrimaryWeaponSchema::Projectile::Delivery::PierceCount, -1.f);
			result.outputMultiplier = definition.attributeOutputMultiplier;
			return result;
		}

		// Ability slot 1 resolves with no weapon, so Primary-only modifiers must not reach it.
		float AbilityDamage(PlayerSpaceShip& ship)
		{
			GameAbility* ability = ship.GetAbilitySystemComponent().GetAbility(sas::AbilitySlot::Ability1);
			if (!ability) return -1.f;
			GameAbilityDefinition definition = ability->GetDefinition();
			AbilityExecutionContext context{ &ship.GetAbilitySystemComponent(), &definition, nullptr, nullptr };
			return sas::FindAttributeValue(AbilityActionAttributeResolver::ResolveAttributes(context, nullptr, Probe()), CommonAttributeIds::Damage, -1.f);
		}

		bool SameValues(const PrimaryValues& left, const PrimaryValues& right)
		{
			return Near(left.damage, right.damage) && Near(left.speed, right.speed) && Near(left.pierce, right.pierce);
		}

		// Sonic on: speed +1100 (x2 on the probe), pierce +1. The resolved Primary damage stat is unchanged; the damage
		// multiplier is applied to the whole Primary shot in Combatant's outgoing damage and is measured by FullShotChecks.
		bool SonicOn(const PrimaryValues& idle, const PrimaryValues& now)
		{
			return Near(now.speed - idle.speed, kProbeSpeed * (2.f - 1.f)) && Near(now.pierce - idle.pierce, 1.f) && Near(now.damage, idle.damage);
		}

		// A real Primary hit (or ability/periodic control) from a ship into a fresh unshielded enemy. Returns the damage dealt.
		// A Primary shot carries the PrimaryShot hit group, exactly as PrimaryWeaponProjectileActor's payload does.
		float Shot(Actor& target, PlayerSpaceShip& ship, float damage, bool primary, DamageDeliveryType delivery = DamageDeliveryType::Direct)
		{
			DamagePayload payload = Plain();
			payload.hitGroup = std::make_shared<DamageHitGroup>(primary ? DamageHitGroup::Kind::PrimaryShot : DamageHitGroup::Kind::AbilityUse);
			return Hit(target, &ship, damage, payload, sas::ContentId{}, primary ? List<GameplayTag>{ GameplayTags::Ability::Primary } : List<GameplayTag>{}, delivery);
		}

		// Whole-shot ratio against the unruled ship, using each ship's own resolved Primary damage (AttackPower scaling included).
		// Ability and periodic Primary-tagged hits must stay at ratio 1.
		void FullShotChecks(Report& report, Fixture& sonic, Fixture& plain, float multiplier, const std::string& label)
		{
			const float sonicDamage = ResolvePrimary(*sonic.ship).damage;
			const float plainDamage = ResolvePrimary(*plain.ship).damage;
			const float sonicShot = Shot(*Enemy(sonic), *sonic.ship, sonicDamage, true);
			const float plainShot = Shot(*Enemy(plain), *plain.ship, plainDamage, true);
			report.Check(plainShot > 0.f && Near(sonicShot / plainShot, multiplier), label,
				Json{ { "ratio", plainShot > 0.f ? sonicShot / plainShot : 0.f }, { "expected", multiplier }, { "sonicShot", sonicShot }, { "plainShot", plainShot } });
			const float sonicAbility = Shot(*Enemy(sonic), *sonic.ship, sonicDamage, false);
			const float plainAbility = Shot(*Enemy(plain), *plain.ship, plainDamage, false);
			report.Check(plainAbility > 0.f && Near(sonicAbility, plainAbility), "ability hits are unaffected by the sonic Primary multiplier",
				Json{ { "sonic", sonicAbility }, { "plain", plainAbility } });
			const float sonicDot = Shot(*Enemy(sonic), *sonic.ship, sonicDamage, true, DamageDeliveryType::Periodic);
			const float plainDot = Shot(*Enemy(plain), *plain.ship, plainDamage, true, DamageDeliveryType::Periodic);
			report.Check(plainDot > 0.f && Near(sonicDot, plainDot), "periodic Primary-tagged ticks are unaffected by the sonic Primary multiplier",
				Json{ { "sonic", sonicDot }, { "plain", plainDot } });
		}

		// One tick at a commanded speed.
		void Drive(Fixture& f, float speed, int ticks)
		{
			for (int tick = 0; tick < ticks; ++tick)
			{
				f.ship->SetVelocity({ speed, 0.f });
				f.level->TickInternal(kDriveStep);
			}
		}

		// The live cap: one clamped tick. The sonic module does not touch the cap, so it can be read before installing.
		float MeasureCap(Fixture& f)
		{
			Drive(f, kFastSpeed, 1);
			return f.ship->GetMovementComponent().GetCurrentMovementSpeed();
		}

		void HoldChecks(Report& report, Application& application, const PrimaryValues& base, Fixture& plain)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			f.ship->SetMovementMode(ShipMovementMode::ThrustDrift);
			const float cap = MeasureCap(f);
			report.Check(cap > 1.f, "the ship's live speed cap measures above zero", Json{ { "cap", cap } });
			const std::size_t handlesIdle = f.ship->GetModuleApplier().GetOwnedHandleCount();
			const std::uint32_t id = f.Install(kSonicId, 0, 1);

			// Below the threshold (50% of cap) for 3 sec: never sonic.
			Drive(f, 0.5f * cap, 30);
			PrimaryValues idle = ResolvePrimary(*f.ship);
			report.Check(SameValues(idle, base), "below 90% of cap for 3 sec leaves Primary values at the unruled baseline");

			// Above the threshold for 1.9 sec: still not sonic.
			Drive(f, kFastSpeed, 19);
			report.Check(SameValues(ResolvePrimary(*f.ship), base), "1.9 sec above 90% of cap is not sonic yet");

			// The 2.0 sec mark: sonic, with the Primary deltas and no change to the ability.
			const float abilityBefore = AbilityDamage(*f.ship);
			Drive(f, kFastSpeed, 1);
			PrimaryValues sonic = ResolvePrimary(*f.ship);
			report.Check(SonicOn(base, sonic), "2 sec above 90% of cap: Primary speed x2, pierce +1, resolved damage stat unchanged",
				Json{ { "speed", sonic.speed }, { "pierce", sonic.pierce }, { "damage", sonic.damage }, { "baseDamage", base.damage } });
			FullShotChecks(report, f, plain, 1.10f, "a full sonic Primary shot deals x1.10 of the unruled shot");
			report.Check(Near(AbilityDamage(*f.ship), abilityBefore) && abilityBefore >= 0.f, "ability damage is unchanged while sonic (Primary-only)");
			report.Check(f.ship->GetModuleApplier().GetOwnedHandleCount() == handlesIdle + 3,
				"sonic holds exactly three Primary modifiers on the applier count",
				Json{ { "idle", handlesIdle }, { "sonic", f.ship->GetModuleApplier().GetOwnedHandleCount() } });

			// The speed drops: no linger on this rule, so sonic ends on the next tick.
			Drive(f, 0.5f * cap, 1);
			report.Check(SameValues(ResolvePrimary(*f.ship), base) && f.ship->GetModuleApplier().GetOwnedHandleCount() == handlesIdle,
				"slowing down ends sonic at once and removes its modifiers");

			// A short dip resets the hold counter: 1.5 sec, a break, then 1.9 sec is still not sonic.
			Drive(f, kFastSpeed, 15);
			Drive(f, 0.5f * cap, 1);
			Drive(f, kFastSpeed, 19);
			report.Check(SameValues(ResolvePrimary(*f.ship), base), "a short dip resets the hold: 1.9 sec after the dip is not sonic");
			Drive(f, kFastSpeed, 1);
			report.Check(SonicOn(base, ResolvePrimary(*f.ship)), "2 sec after the dip is sonic again");
		}

		void LevelChecks(Report& report, Application& application, const PrimaryValues& base, Fixture& plain, int level, float multiplier)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			f.ship->SetMovementMode(ShipMovementMode::ThrustDrift);
			const float cap = MeasureCap(f);
			f.Install(kSonicId, 0, level);
			Drive(f, kFastSpeed, 20);
			const PrimaryValues sonic = ResolvePrimary(*f.ship);
			report.Check(SonicOn(base, sonic), "level " + std::to_string(level) + " sonic keeps the speed and pierce deltas",
				Json{ { "damage", sonic.damage }, { "baseDamage", base.damage }, { "cap", cap } });
			FullShotChecks(report, f, plain, multiplier, "level " + std::to_string(level) + " sonic full shot uses its own damage multiplier");
		}

		void LingerChecks(Report& report, Application& application, const PrimaryValues& base, Fixture& plain)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			f.ship->SetMovementMode(ShipMovementMode::ThrustDrift);
			const float cap = MeasureCap(f);
			f.Install(kLingerId, 0, 1);
			Drive(f, kFastSpeed, 20);
			report.Check(SonicOn(base, ResolvePrimary(*f.ship)), "linger rule: sonic after 2 sec above the threshold");
			FullShotChecks(report, f, plain, 1.10f, "linger rule: scalar damage multiplier applies to the full shot");

			// 1.4 sec below the threshold: still sonic inside the 1.5 sec linger.
			Drive(f, 0.5f * cap, 14);
			report.Check(SonicOn(base, ResolvePrimary(*f.ship)), "linger rule: still sonic 1.4 sec after the speed dropped");

			// 1.6 sec below: the linger has run out.
			Drive(f, 0.5f * cap, 2);
			report.Check(SameValues(ResolvePrimary(*f.ship), base), "linger rule: sonic ends after 1.5 sec below the threshold");
		}

		void RemovalAndCycleChecks(Report& report, Application& application, const PrimaryValues& base)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			f.ship->SetMovementMode(ShipMovementMode::ThrustDrift);
			MeasureCap(f);
			const std::size_t handlesIdle = f.ship->GetModuleApplier().GetOwnedHandleCount();
			const std::uint32_t id = f.Install(kSonicId, 0, 1);
			const ModulePlacement placed = *f.loadout.FindPlacement(id);
			Drive(f, kFastSpeed, 20);
			report.Check(SonicOn(base, ResolvePrimary(*f.ship)), "sonic is active before the module is removed");

			// Removing the module ends sonic at once, with no tick in between.
			f.Remove(id);
			report.Check(SameValues(ResolvePrimary(*f.ship), base) && f.ship->GetModuleApplier().GetOwnedHandleCount() == handlesIdle,
				"removing the module ends sonic at once and leaves no Primary modifier");
			report.Check(f.loadout.Install(f.rules, id, placed.origin, placed.rotationQuarterTurns) == ModulePlacementFailure::None &&
				f.ship->GetModuleApplier().Apply(f.loadout, f.rules), "the module reinstalls at its original cell before the cycles");

			// Ten remove and reinstall cycles: counts stay fixed, and every sonic reading is the same.
			int checked = 0;
			bool cyclesOk = true;
			for (int cycle = 0; cycle < 10; ++cycle)
			{
				Drive(f, kFastSpeed, 20);
				cyclesOk = cyclesOk && SonicOn(base, ResolvePrimary(*f.ship)) &&
					f.ship->GetModuleApplier().GetOwnedHandleCount() == handlesIdle + 3;
				++checked;
				cyclesOk = cyclesOk && f.loadout.Uninstall(id);
				f.ship->GetModuleApplier().Apply(f.loadout, f.rules);
				cyclesOk = cyclesOk && f.loadout.Install(f.rules, id, placed.origin, placed.rotationQuarterTurns) == ModulePlacementFailure::None;
				f.ship->GetModuleApplier().Apply(f.loadout, f.rules);
				cyclesOk = cyclesOk && f.ship->GetModuleApplier().GetOwnedHandleCount() == handlesIdle;
			}
			report.Check(cyclesOk && checked == 10 && SameValues(ResolvePrimary(*f.ship), base),
				"ten remove and reinstall cycles keep the handle count fixed and every sonic reading identical",
				Json{ { "idle", handlesIdle }, { "cycles", checked } });
		}

		// One real Primary shot, the way the game fires it (same pattern as CommonModuleE2E_FullMagazine): FireOnce with the
		// attributes the fire action resolves, the spawned projectile, then its real overlap impact on a fresh enemy-layer
		// target. The source-side damage record of that impact is returned; landed is false when nothing was hit.
		struct LiveShot
		{
			bool landed = false;
			float damage = 0.f;
			bool critical = false;
			List<GameplayTag> sourceAbilityTags;
			// Every impact record of the shot, in order: the Primary hit first, then any secondary hits it spawned.
			List<common_module_e2e::CapturedDamage> records;
		};

		LiveShot FireLiveShot(Fixture& f)
		{
			LiveShot shot;
			const PrimaryWeaponDefinition* catalog = content::WeaponContentCatalog::FindById("Weapon.Projectile.FighterRapidLaser.Basic");
			GameAbility* primary = f.ship->GetAbilitySystemComponent().GetAbility(sas::AbilitySlot::PrimaryFire);
			if (!catalog || !primary) return shot;
			const PrimaryWeaponDefinition weapon = *catalog;
			GameAbilityDefinition definition = primary->GetDefinition();
			AbilityExecutionContext resolve{ &f.ship->GetAbilitySystemComponent(), &definition, nullptr, nullptr };
			sas::GameplayAttributeList attributes = AbilityActionAttributeResolver::ResolveAttributes(resolve, &weapon, weapon.attributes);
			PrimaryWeaponExecutionContext fire{ *f.ship, weapon, attributes };
			fire.damageTags = weapon.damageTags;
			fire.shouldContinue = [] { return true; };
			PrimaryWeaponRuntimeState runtime;
			PrimaryWeaponExecutionSystem::InitializeRuntime(weapon, runtime);
			PrimaryWeaponExecutionSystem::BeginFire(fire, runtime);
			const std::size_t before = f.level->GetActorsByType<PrimaryWeaponProjectileActor>().size();
			const bool fired = PrimaryWeaponExecutionSystem::FireOnce(fire, runtime);
			PrimaryWeaponExecutionSystem::EndFire(fire, runtime);
			f.level->TickInternal(0.f);
			const auto projectiles = f.level->GetActorsByType<PrimaryWeaponProjectileActor>();
			if (!fired || projectiles.size() != before + 1) return shot;
			const shared_ptr<PrimaryWeaponProjectileActor> projectile = projectiles.back().lock();
			if (!projectile) return shot;

			const auto target = f.level->SpawnActor<SpaceShip>(content::ShipContentCatalog::GetPlayerFighterDefinition()).lock();
			f.level->TickInternal(0.f);
			target->SetCollisionLayer(vega::GameCollision::Enemy);
			target->SetCollisionMask(vega::GameCollision::PlayerBullet);
			target->GetShieldComponent().ChangeShield(-10000.f);
			target->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Armor, 0.f);
			target->GetHealthComponent().SetMaxHealth(5000.f, true);

			common_module_e2e::DamageCapture capture;
			const DelegateHandle handle = f.ship->GetCombatRuntime().onDamageDealt.BindAction(&capture, &common_module_e2e::DamageCapture::OnDamage);
			projectile->OnActorBeginOverlap(target.get());
			f.ship->GetCombatRuntime().onDamageDealt.UnbindAction(handle);
			if (capture.records.empty()) return shot;
			const auto& record = capture.records.front();
			shot.landed = true;
			shot.damage = record.originalDamage;
			shot.critical = record.wasCritical;
			shot.sourceAbilityTags = record.sourceAbilityTags;
			shot.records = capture.records;
			return shot;
		}

		// Critical hits are pinned off so the live comparison is exact.
		void NoCrits(Fixture& f)
		{
			f.ship->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::CriticalChance, 0.f);
		}

		void LiveShotChecks(Report& report, Application& application)
		{
			Fixture plain{ report, application };
			plain.rules.lookup = FixtureLookup;
			plain.ship->SetMovementMode(ShipMovementMode::ThrustDrift);
			MeasureCap(plain);
			NoCrits(plain);
			const LiveShot plainShot = FireLiveShot(plain);
			report.Check(plainShot.landed && plainShot.damage > 0.f, "a real fired Primary projectile lands on a live enemy-layer target",
				Json{ { "damage", plainShot.damage } });

			Fixture sonic{ report, application };
			sonic.rules.lookup = FixtureLookup;
			sonic.ship->SetMovementMode(ShipMovementMode::ThrustDrift);
			MeasureCap(sonic);
			NoCrits(sonic);
			sonic.Install(kSonicId, 0, 1);
			Drive(sonic, kFastSpeed, 20);
			report.Check(ResolvePrimary(*sonic.ship).speed > ResolvePrimary(*plain.ship).speed + 1.f, "the live sonic ship is sonic before it fires");
			const LiveShot sonicShot = FireLiveShot(sonic);
			report.Check(sonicShot.landed && !sonicShot.critical && plainShot.damage > 0.f && sonicShot.damage >= plainShot.damage * 1.10f - 0.01f && sonicShot.damage <= plainShot.damage * 1.10f + 1.01f,
				"a live Primary projectile from a sonic ship lands x1.10 on the target",
				Json{ { "sonicDamage", sonicShot.damage }, { "plainDamage", plainShot.damage }, { "landed", sonicShot.landed } });

			// Probe, recorded not asserted: does a PrimaryShotOnly module multiplier reach a live Primary projectile impact?
			Fixture probe{ report, application };
			probe.rules.lookup = FixtureLookup;
			probe.ship->SetMovementMode(ShipMovementMode::ThrustDrift);
			MeasureCap(probe);
			NoCrits(probe);
			probe.Install(kPrimaryOnlyId, 0, 1);
			const LiveShot probeShot = FireLiveShot(probe);
			const bool sourcePrimaryTag = std::any_of(probeShot.sourceAbilityTags.begin(), probeShot.sourceAbilityTags.end(),
				[](const GameplayTag& tag) { return tag.MatchesTag(GameplayTags::Ability::Primary); });
			const float observed = probeShot.landed && plainShot.damage > 0.f ? probeShot.damage / plainShot.damage : 0.f;
			report.artifact["primaryShotOnlyLiveProbe"] = Json{ { "expectedIfLive", 1.5 }, { "observedRatio", observed },
				{ "landed", probeShot.landed }, { "liveSourceTagCount", probeShot.sourceAbilityTags.size() },
				{ "liveSourceTagsIncludePrimary", sourcePrimaryTag }, { "matchedLive", std::abs(observed - 1.5f) <= 0.05f } };
			report.Check(probeShot.landed && std::abs(observed - 1.5f) <= 0.05f,
				"a PrimaryShotOnly module multiplier reaches a live Primary projectile impact (x1.5)",
				Json{ { "observedRatio", observed }, { "liveSourceTagsIncludePrimary", sourcePrimaryTag } });

			// Shatter Current (shipped): after a real shield break, the next live Primary hit adds an Energy secondary of 30%
			// of its raw damage. The Primary hit itself stays at the plain damage (the rule has no damage multiplier).
			Fixture shatter{ report, application };
			shatter.rules.lookup = FixtureLookup;
			shatter.ship->SetMovementMode(ShipMovementMode::ThrustDrift);
			MeasureCap(shatter);
			NoCrits(shatter);
			shatter.Install("Module.Defense.ShatterCurrent.Basic", 0, 1);
			const auto breaker = Enemy(shatter);
			shatter.ship->GetShieldComponent().SetMaxShield(100.f);
			shatter.ship->GetShieldComponent().ChangeShield(100.f);
			ApplyCombatDamage(*shatter.ship, 101.f, breaker.get(), { GameplayTags::Damage::Type::Kinetic }, Plain());
			report.Check(shatter.ship->GetShieldComponent().GetShield() <= 0.f, "a real hit breaks the shield before the Shatter Current check");
			const LiveShot shatterShot = FireLiveShot(shatter);
			const bool energySecondary = shatterShot.records.size() == 2 && shatterShot.records[1].damageTags.size() == 1 &&
				shatterShot.records[1].damageTags.front() == GameplayTags::Damage::Type::Energy;
			report.Check(energySecondary && Near(shatterShot.records[0].originalDamage, plainShot.damage) &&
				std::abs(shatterShot.records[1].originalDamage - 0.3f * plainShot.damage) <= 0.02f * plainShot.damage,
				"a live Primary hit after a shield break adds a 30% Energy secondary (Shatter Current)",
				Json{ { "records", shatterShot.records.size() }, { "primary", shatterShot.records.empty() ? 0.f : shatterShot.records[0].originalDamage },
					{ "secondary", shatterShot.records.size() > 1 ? shatterShot.records[1].originalDamage : 0.f }, { "plainDamage", plainShot.damage } });
		}

		void LoaderChecks(Report& report, const std::filesystem::path& root, const Json& shipped)
		{
			const Json valid = SonicRules()[0];
			const auto reject = [&](const char* name, const Json& rules)
			{
				const auto path = WriteJson(root, std::string{ "reject-" } + name + ".json", FixtureSet(shipped, rules, SonicRules()));
				const auto loaded = content::ModuleLoader::LoadFromFile(path);
				report.Check(!loaded.Succeeded(), std::string{ "sustainedSpeedRules loader rejects " } + name, loaded.error);
			};
			const auto withField = [&valid](const char* key, Json value)
			{
				Json rule = valid;
				rule[key] = std::move(value);
				return Json::array({ rule });
			};
			const auto withoutField = [&valid](const char* key)
			{
				Json rule = valid;
				rule.erase(key);
				return Json::array({ rule });
			};
			reject("fraction-zero", withField("speedFraction", 0.0));
			reject("fraction-above-one", withField("speedFraction", 1.5));
			reject("fraction-missing", withoutField("speedFraction"));
			reject("hold-negative", withField("holdSeconds", -1.0));
			reject("hold-missing", withoutField("holdSeconds"));
			reject("speed-multiplier-below-one", withField("primarySpeedMultiplier", 0.9));
			reject("pierce-negative", withField("primaryPierceBonus", -1.0));
			reject("damage-below-one", withField("primaryDamageMultiplier", 0.9));
			reject("damage-table-entry-below-one", withField("primaryDamageMultiplier", Json::array({ 1.1, 0.9, 1.2 })));
			reject("damage-short-table", withField("primaryDamageMultiplier", Json::array({ 1.1, 1.2 })));
			reject("linger-negative", withField("lingerSeconds", -1.0));
			reject("unknown-field", withField("bonus", 1.0));
			reject("no-primary-effect", Json::array({ Json{ { "speedFraction", 0.9 }, { "holdSeconds", 2.0 } } }));
			reject("rules-not-array", Json::object());
			reject("entry-not-object", Json::array({ 1.0 }));

			const auto accepted = WriteJson(root, "accept-sonic.json", FixtureSet(shipped, SonicRules(), LingerRules()));
			const auto loaded = content::ModuleLoader::LoadFromFile(accepted);
			report.Check(loaded.Succeeded(), "a valid sustained speed rule loads through the real ModuleLoader", loaded.error);
			const auto found = std::find_if(loaded.definitions.begin(), loaded.definitions.end(),
				[](const ModuleDefinition& definition) { return definition.moduleId == sas::ContentId{ kSonicId }; });
			report.Check(found != loaded.definitions.end() && found->sizeVariants.size() == 1 && found->sizeVariants[0].sustainedSpeedRules.size() == 1,
				"the accepted rule is kept on the size variant");
			if (found != loaded.definitions.end() && !found->sizeVariants.empty() && !found->sizeVariants[0].sustainedSpeedRules.empty())
			{
				const ModuleSustainedSpeedRule& rule = found->sizeVariants[0].sustainedSpeedRules[0];
				report.Check(rule.speedFraction == 0.9f && rule.holdSeconds == 2.f && rule.primarySpeedMultiplier == 2.f && rule.primaryPierceBonus == 1.f &&
					rule.lingerSeconds == 0.f && rule.primaryDamageMultiplierByLevel.values.size() == 3 && rule.primaryDamageMultiplierByLevel.values[2] == 1.2f,
					"the accepted rule keeps its parsed values and the per-level damage table");
			}

			const auto shippedLoaded = content::ModuleLoader::LoadFromFile(
				std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/modules.json");
			report.Check(shippedLoaded.Succeeded(), "shipped modules still load through the sustainedSpeedRules parser", shippedLoaded.error);
		}
	}
}

int main(int argc, char** argv)
{
	// Setup below runs before any Application exists; applications adopt this binding.
	vega::EngineContext testContext;
	vega::EngineContext::Scope testScope{ testContext };
	vega::InstallVegaEngineAssets();
	using namespace rare_sustained_speed_e2e;
	if (argc != 2) return 2;
	Report report;
	try
	{
		vega::EngineContext::Current().Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Rare sustained speed content registration failed" };
		std::string title = "Rare Sustained Speed E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);

		const std::filesystem::path root = std::filesystem::path{ argv[1] }.parent_path() / "rare-sustained-speed-fixtures";
		Json shipped;
		{
			std::ifstream input{ std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/modules.json" };
			input >> shipped;
		}
		const auto fixturePath = WriteJson(root, "rare-sustained-speed-modules.json", FixtureSet(shipped, SonicRules(), LingerRules()));
		const auto loaded = content::ModuleLoader::LoadFromFile(fixturePath);
		report.Check(loaded.Succeeded(), "sustained speed fixture modules load through the real ModuleLoader", loaded.error);
		gFixtureDefinitions = loaded.definitions;

		// The unruled baseline: a ship with no module, read through the same resolver.
		Fixture plain{ report, application };
		plain.rules.lookup = FixtureLookup;
		plain.ship->SetMovementMode(ShipMovementMode::ThrustDrift);
		const PrimaryValues base = ResolvePrimary(*plain.ship);
		report.Check(base.damage > 0.f && base.speed > 0.f, "the baseline ship exposes a live Primary weapon",
			Json{ { "damage", base.damage }, { "speed", base.speed }, { "pierce", base.pierce } });

		HoldChecks(report, application, base, plain);
		LevelChecks(report, application, base, plain, 2, 1.15f);
		LevelChecks(report, application, base, plain, 3, 1.20f);
		LingerChecks(report, application, base, plain);
		RemovalAndCycleChecks(report, application, base);
		LiveShotChecks(report, application);
		LoaderChecks(report, root, shipped);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& exception) { report.artifact["error"] = exception.what(); std::cerr << exception.what() << '\n'; }
	report.artifact["test"] = "RareSustainedSpeedE2E";
	WriteArtifact(argv[1], report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
