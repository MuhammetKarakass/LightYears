#include "framework/EngineContext.h"
#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP6E2E.h"
#include "gameplay/ability/rocket/RocketProjectileActor.h"
#include "gameplay/ability/infernoSpray/InfernoSprayActor.h"
#include "gameplay/ability/combatSentry/CombatSentryContracts.h"
#include "gameplay/ability/combatSentry/CombatSentryActor.h"
#include "gameplay/ability/inertialWake/InertialWakeActor.h"
#include "gameplay/ability/inertialWake/InertialWakeContracts.h"
#include "gameplay/ability/overdriveCore/OverdriveCoreContracts.h"
#include "gameplay/ability/executionDrive/ExecutionDriveContracts.h"
#include "gameplay/ability/glacialPressure/GlacialPressureContracts.h"
#include "gameplay/ability/runtime/MainEffectMultiplier.h"
#include "gameplay/tags/GameplayTagSchema.h"

namespace
{
	using namespace uncommon_p6_e2e;
	constexpr const char* Primed = "Module.Offense.PrimedArsenal.Legendary";
	constexpr const char* Hardlight = "Module.Defense.HardlightHull.Legendary";
	constexpr const char* Rocket = "Ability.Offense.Rocket.Basic";

	GameAbility& Grant(Fixture& f, const char* id, sas::AbilitySlot slot)
	{
		auto& asc = f.ship->GetAbilitySystemComponent();
		if (const auto* old = asc.GetAbility(slot)) f.report.Check(asc.RemoveAbility(old->GetHandle()), "replace selected slot");
		if (const auto* old = asc.GetAbilityById(id)) f.report.Check(asc.RemoveAbility(old->GetHandle()), "move existing content identity");
		auto definition = *content::AbilityContentCatalog::FindById(id); definition.slot = slot;
		f.report.Check(asc.GrantAbility(definition).IsValid(), "grant shipped ability");
		return *asc.GetAbility(slot);
	}

	void Ready(GameAbility& ability)
	{
		if (ability.IsActive()) ability.Cancel(sas::AbilityEndReason::Completed);
		ability.Tick(1000.f);
	}

	shared_ptr<RocketProjectileActor> Fire(Fixture& f, GameAbility& ability)
	{
		for (const auto& actor : Actors<RocketProjectileActor>(f)) actor->Destroy();
		f.report.Check(ability.TryActivate(), "natural activation succeeds");
		const auto actors = Actors<RocketProjectileActor>(f);
		f.report.Check(actors.size() == 1, "one real Rocket projectile");
		return actors.front();
	}

	std::uint32_t Install(Fixture& f, const ModuleRulesContext& rules, const char* id, int level)
	{
		const auto& module = *rules.lookup(id);
		const auto instance = f.loadout.AddToInventory(rules, module, 0, module.rarity, level);
		const auto placement = FindPlacement(rules, f.loadout, module, instance);
		f.report.Check(instance && placement.has_value(), "find legal Legendary placement");
		f.report.Check(f.loadout.Install(rules, instance, placement->first, placement->second) == ModulePlacementFailure::None, "install shipped Legendary");
		f.report.Check(f.ship->GetModuleApplier().Apply(f.loadout, rules), "apply through real ship runtime");
		return instance;
	}

	ShipSchematic Schematic(const GameplayTag& zone)
	{
		ShipSchematic schematic; schematic.width = 20; schematic.height = 20; schematic.cellZones.assign(400, zone);
		return schematic;
	}

	void PrimedLevels(Report& report, Application& application)
	{
		for (int level = 1; level <= 5; ++level)
		{
			Fixture f{ report, application };
			auto& asc = f.ship->GetAbilitySystemComponent();
			auto& rocket = Grant(f, Rocket, sas::AbilitySlot::Ability4);
			const auto baseline = Fire(f, rocket);
			const float damage = baseline->GetDamage(), radius = baseline->GetExplosionRadius(), speed = baseline->GetProjectileSpeed(), range = baseline->GetMaximumRange(), cooldown = rocket.GetCooldownDuration();
			const auto basePayload = baseline->GetDamagePayload();
			Ready(rocket);
			const auto schematic = Schematic(level > 3 ? GameplayTags::Module::Zone::Hull : GameplayTags::Module::Zone::Nose);
			const ModuleRulesContext rules{ schematic, f.rules.lookup, f.rules.zoneTable, f.rules.sideFeatureLookup };
			const auto instance = Install(f, rules, Primed, level > 3 ? level - 2 : level);
			auto& applier = f.ship->GetModuleApplier();
			const auto& runtime = applier.GetLegendaryPrimedArsenalRuntime();
			const float rate = .15f + .05f * (level - 1), cap = 1.f + rate * (20.f / 3.f);
			f.Equal("effective-level rate from JSON", rate, runtime.GetRules().front().powerPerReadySecond);
			applier.Tick(2.f);
			f.Equal("ready reserve accumulates by elapsed seconds", 1.f + 2.f * rate, runtime.GetMultiplier(rocket.GetHandle()));
			applier.Tick(100.f);
			f.Equal("fixed six and two thirds second cap at every level", cap, runtime.GetMultiplier(rocket.GetHandle()));
			asc.AddOwnedTag(GameplayTagSchema::BlockAbilityActivation);
			report.Check(!rocket.TryActivate(), "blocked activation fails");
			f.Equal("failed use preserves reserve", cap, runtime.GetMultiplier(rocket.GetHandle()));
			asc.RemoveOwnedTag(GameplayTagSchema::BlockAbilityActivation);
			const auto powered = Fire(f, rocket);
			f.Equal("main effect snapshots at successful use", cap, rocket.GetDefinition().mainEffectMultiplier);
			f.Equal("real projectile damage multiplied once", damage * cap, powered->GetDamage());
			f.Equal("reserve resets on commit", 1.f, runtime.GetMultiplier(rocket.GetHandle()));
			f.Equal("radius unchanged", radius, powered->GetExplosionRadius());
			f.Equal("projectile speed unchanged", speed, powered->GetProjectileSpeed());
			f.Equal("range unchanged", range, powered->GetMaximumRange());
			f.Equal("cooldown unchanged", cooldown, rocket.GetCooldownDuration());
			const auto payload = powered->GetDamagePayload();
			report.Check(payload.igniteStacks == basePayload.igniteStacks && payload.electricStacks == basePayload.electricStacks && payload.kineticStacks == basePayload.kineticStacks, "status stacks unchanged");
			applier.Tick(100.f);
			f.Equal("cooldown never accumulates", 1.f, runtime.GetMultiplier(rocket.GetHandle()));
			f.Equal("launched output keeps snapshot", damage * cap, powered->GetDamage());
			Ready(rocket); applier.Tick(1.f);
			report.Check(f.loadout.Uninstall(instance) && applier.Apply(f.loadout, rules), "uninstall Primed");
			f.Equal("uninstall clears reserve", 1.f, runtime.GetMultiplier(rocket.GetHandle()));
		}
	}

	void ChannelAndReplay(Report& report, Application& application)
	{
		Fixture f{ report, application };
		auto& asc = f.ship->GetAbilitySystemComponent();
		auto& beam = Grant(f, "Ability.Offense.InfernoSpray.Basic", sas::AbilitySlot::Ability4);
		report.Check(beam.TryActivate(), "baseline channel starts");
		const auto baseline = Actors<InfernoSprayActor>(f);
		report.Check(!baseline.empty(), "channel spawns real beam");
		const float baseDamage = baseline.back()->GetDamage(), duration = beam.GetActiveDuration();
		Ready(beam);
		const auto primed = f.Install(Primed);
		auto& applier = f.ship->GetModuleApplier();
		const auto rule = applier.GetLegendaryPrimedArsenalRuntime().GetRules().front();
		applier.Tick(rule.readyWindowSeconds);
		report.Check(beam.TryActivate() && beam.IsActive(), "powered channel starts");
		const float multiplier = 1.f + rule.powerPerReadySecond * rule.readyWindowSeconds;
		const auto beams = Actors<InfernoSprayActor>(f);
		f.Equal("whole channel tick damage snapshots", baseDamage * multiplier, beams.back()->GetDamage());
		f.Equal("channel duration unchanged", duration, beam.GetActiveDuration());
		applier.Tick(20.f);
		f.Equal("active channel never accumulates", 1.f, applier.GetLegendaryPrimedArsenalRuntime().GetMultiplier(beam.GetHandle()));
		f.Equal("later ticks retain channel power", baseDamage * multiplier, beams.back()->GetDamage());
		const auto* recorded = asc.GetAbilityUseHistory().FindLatestUnconsumed();
		report.Check(recorded != nullptr, "successful natural use enters history");
		const auto record = *recorded;
		Ready(beam); applier.Tick(1.f);
		const float reserve = applier.GetLegendaryPrimedArsenalRuntime().GetMultiplier(beam.GetHandle());
		std::string failure;
		report.Check(asc.InvokeRecordedAbility(record, {}, .5f, sas::AbilitySlot::Ability1, false, &failure), "replay invokes recorded channel", failure);
		f.Equal("replay preserves the original natural reserve", reserve, applier.GetLegendaryPrimedArsenalRuntime().GetMultiplier(beam.GetHandle()));
		const auto replay = Actors<InfernoSprayActor>(f);
		f.Equal("replay copies original power once then applies replay power", baseDamage * multiplier * .5f, replay.back()->GetDamage());
		f.Remove(primed);
	}

	void ChargesAndIndependentReserves(Report& report, Application& application)
	{
		Fixture f{ report, application };
		auto& asc = f.ship->GetAbilitySystemComponent();
		auto& rocket = Grant(f, Rocket, sas::AbilitySlot::Ability4);
		auto& beam = Grant(f, "Ability.Offense.InfernoSpray.Basic", sas::AbilitySlot::Ability1);
		f.Install(Primed);
		report.Check(asc.SetAbilityChargeBonus(std::string{ Rocket }, 2), "extend one ability through the real charge API");
		auto& applier = f.ship->GetModuleApplier();
		const auto& runtime = applier.GetLegendaryPrimedArsenalRuntime();
		const float rate = runtime.GetRules().front().powerPerReadySecond;
		applier.Tick(2.f);
		Fire(f, rocket);
		f.Equal("all charges share one reserve", 1.f, runtime.GetMultiplier(rocket.GetHandle()));
		f.Equal("another ability instance retains its independent reserve", 1.f + 2.f * rate, runtime.GetMultiplier(beam.GetHandle()));
		applier.Tick(10.f);
		f.Equal("partly charged ability cannot accumulate while recharging", 1.f, runtime.GetMultiplier(rocket.GetHandle()));
		rocket.Tick(rocket.GetDefinition().chargeUseInterval);
		Fire(f, rocket);
		f.Equal("second charge does not reuse consumed power", 1.f, rocket.GetDefinition().mainEffectMultiplier);
		Ready(rocket); applier.Tick(1.f);
		f.Equal("fully recharged ability accumulates again", 1.f + rate, runtime.GetMultiplier(rocket.GetHandle()));
		const auto previous = rocket.GetHandle();
		report.Check(asc.RemoveAbility(previous), "remove a charged ability instance");
		auto& replacement = Grant(f, Rocket, sas::AbilitySlot::Ability4);
		f.Equal("replacement content identity does not inherit the removed instance reserve", 1.f, runtime.GetMultiplier(replacement.GetHandle()));
	}

	void SpecialOutputs(Report& report, Application& application)
	{
		Fixture f{ report, application };
		f.ship->GetMovementComponent().SetMovementMode(ShipMovementMode::ThrustDrift);
		auto& wake = Grant(f, "Ability.Offense.InertialWake.Basic", sas::AbilitySlot::Ability4);
		AbilityExecutionContext baseContext{ &f.ship->GetAbilitySystemComponent(), &wake.GetDefinition(), nullptr, &wake };
		const auto base = AbilityActionAttributeResolver::ResolveAbilityAttributes(baseContext);
		f.Install(Primed);
		auto& applier = f.ship->GetModuleApplier();
		const auto rule = applier.GetLegendaryPrimedArsenalRuntime().GetRules().front();
		const float power = 1.f + rule.powerPerReadySecond * rule.readyWindowSeconds;
		applier.Tick(rule.readyWindowSeconds);
		report.Check(wake.TryActivate(), "powered speed-derived offense starts");
		const auto actors = Actors<InertialWakeActor>(f);
		report.Check(actors.size() == 1, "real speed-derived damage actor spawns");
		f.Equal("speed-derived damage retains Primed snapshot", power, actors.front()->GetDamagePayload().mainEffectMultiplier);
		AbilityExecutionContext context{ &f.ship->GetAbilitySystemComponent(), &wake.GetDefinition(), nullptr, &wake };
		const auto powered = AbilityActionAttributeResolver::ResolveAbilityAttributes(context);
		for (const auto& id : { AbilityData::InertialWake::Attribute::TopSpeedBonus, AbilityData::InertialWake::Attribute::ThrustBonus })
			f.Equal("Primed does not amplify speed or thrust", sas::FindAttributeValue(base, id), sas::FindAttributeValue(powered, id));

		Fixture summon{ report, application };
		auto& sentry = Grant(summon, "Ability.Offense.CombatSentry.Basic", sas::AbilitySlot::Ability4);
		const auto* definition = AbilityData::FindAbilityActorDefinition("Actor.Ability.CombatSentry.Turret.Basic");
		report.Check(definition != nullptr, "shipped damage-scaling summon definition");
		AbilityExecutionContext sentryContext{ &summon.ship->GetAbilitySystemComponent(), &sentry.GetDefinition(), nullptr, &sentry };
		const auto original = AbilityActionAttributeResolver::ResolveAttributes(sentryContext, nullptr, definition->attributes);
		summon.Install(Primed);
		const auto summonRule = summon.ship->GetModuleApplier().GetLegendaryPrimedArsenalRuntime().GetRules().front();
		summon.ship->GetModuleApplier().Tick(summonRule.readyWindowSeconds);
		report.Check(sentry.TryActivate(), "powered real summon starts");
		const auto resolved = AbilityActionAttributeResolver::ResolveAttributes(sentryContext, nullptr, definition->attributes);
		const float summonPower = 1.f + summonRule.powerPerReadySecond * summonRule.readyWindowSeconds;
		for (const auto& id : { AbilityData::CombatSentry::Actor::Turret::BaseDamage, AbilityData::CombatSentry::Actor::Turret::OwnerAttackPowerScale })
			summon.Equal("summon attack formula scales once", sas::FindAttributeValue(original, id) * summonPower, sas::FindAttributeValue(resolved, id));
		for (const auto& id : { AbilityData::CombatSentry::Actor::Turret::MaxHealth, AbilityData::CombatSentry::Actor::Turret::AttackRate, AbilityData::CombatSentry::Actor::Turret::TargetingRange })
			summon.Equal("summon health attack rate and range remain unchanged", sas::FindAttributeValue(original, id), sas::FindAttributeValue(resolved, id));
	}

	void GroupAndBuff(Report& report, Application& application)
	{
		Fixture f{ report, application };
		auto& rocket = Grant(f, Rocket, sas::AbilitySlot::Ability4);
		const auto base = Fire(f, rocket); const float baseDamage = base->GetDamage(); Ready(rocket);
		const auto primed = f.Install(Primed);
		const auto cascade = f.Install("Module.Utility.CascadeReactor.Epic");
		f.Install("Module.Offense.MonochromeCore.Epic");
		Grant(f, "Ability.Offense.InertialWake.Basic", sas::AbilitySlot::Ability1);
		ModulePlacement shell;
		for (const auto& placement : f.loadout.GetPlacements()) if (AbilitySlotForModuleSystem(f.rules.lookup(placement.moduleId)->systemCategory) == sas::AbilitySlot::Ability4) shell = placement;
		report.Check(shell.instanceId != 0, "find standard R shell");
		f.Remove(shell.instanceId);
		const auto& ultimate = *f.rules.lookup("Module.AbilityModule.UltimateProtocol");
		const auto ultimateId = f.loadout.AddToInventory(f.rules, ultimate, 0, ultimate.rarity, 1);
		report.Check(f.loadout.Install(f.rules, ultimateId, shell.origin, 0) == ModulePlacementFailure::None && f.ship->GetModuleApplier().Apply(f.loadout, f.rules), "install real Ultimate Protocol");
		auto& applier = f.ship->GetModuleApplier();
		const auto primeRule = applier.GetLegendaryPrimedArsenalRuntime().GetRules().front();
		applier.Tick(primeRule.readyWindowSeconds);
		CastMovement(f);
		const float cascadeBonus = applier.GetResolvedEffects().epicRules.cascades.front().stepBonus;
		const float primeBonus = primeRule.powerPerReadySecond * primeRule.readyWindowSeconds;
		const auto projectile = Fire(f, rocket);
		const float group = 1.f + primeBonus + .6f + cascadeBonus;
		f.Equal("Primed Ultimate Cascade share one additive snapshot", group, rocket.GetDefinition().mainEffectMultiplier);
		f.Equal("snapshot drives actual output", baseDamage * group, projectile->GetDamage());
		float mono = 1.f;
		for (const auto& [tag, multiplier] : f.ship->GetCombatRuntime().GetEpicOffenseEffects().monochromeMultipliers)
			for (const auto& damageTag : rocket.GetResolvedDamageTags()) if (damageTag.MatchesTag(tag)) mono = std::max(mono, multiplier);
		report.Check(mono > 1.f, "real matching loadout enables Monochrome");
		const auto enemy = Enemy(f);
		auto payload = Plain(); payload.hitGroup = rocket.GetDamageHitGroup();
		const float delivered = Hit(*enemy, f.ship.get(), 10.f * group, payload, Rocket, rocket.GetDefinition().abilityTags, DamageDeliveryType::Projectile, rocket.GetResolvedDamageTags());
		f.Equal("Monochrome joins all three additively in real damage pipeline", 10.f * (group + mono - 1.f), delivered);
		f.Remove(primed);
		Ready(rocket); CastMovement(f);
		const auto unprimed = Fire(f, rocket);
		f.Equal("existing Ultimate and third Cascade step add instead of multiply", 1.6f + 3.f * cascadeBonus, rocket.GetDefinition().mainEffectMultiplier);
		const auto unprimedRecord = *f.ship->GetAbilitySystemComponent().GetAbilityUseHistory().FindLatestUnconsumed();
		f.Remove(cascade);
		f.Remove(ultimateId);
		report.Check(f.loadout.Install(f.rules, shell.instanceId, shell.origin, shell.rotationQuarterTurns) == ModulePlacementFailure::None && applier.Apply(f.loadout, f.rules), "restore standard R shell after Ultimate removal");
		for (const auto& actor : Actors<RocketProjectileActor>(f)) actor->Destroy();
		std::string failure;
		report.Check(f.ship->GetAbilitySystemComponent().InvokeRecordedAbility(unprimedRecord, rocket.GetDefinition().scalingRules, .5f, sas::AbilitySlot::Ability1, false, &failure), "replay without Primed retains existing resolution", failure);
		const auto replay = Actors<RocketProjectileActor>(f);
		report.Check(replay.size() == 1, "unprimed replay spawns one projectile");
		f.Equal("unprimed replay does not retain removed Ultimate or Cascade", baseDamage * .5f, replay.front()->GetDamage());

		Fixture buffs{ report, application };
		auto& drive = Grant(buffs, "Ability.Offense.ExecutionDrive.Basic", sas::AbilitySlot::Ability4);
		const float ap = buffs.Owner(OwnerAttributeIds::AttackPower);
		report.Check(drive.TryActivate(), "baseline AP buff starts");
		const float baseBonus = buffs.Owner(OwnerAttributeIds::AttackPower) - ap;
		AbilityExecutionContext baseContext{ &buffs.ship->GetAbilitySystemComponent(), &drive.GetDefinition(), nullptr, &drive };
		const float move = sas::FindAttributeValue(AbilityActionAttributeResolver::ResolveAbilityAttributes(baseContext), AbilityData::ExecutionDrive::Attribute::MoveSpeedBonus);
		Ready(drive); buffs.Install(Primed);
		const auto buffRule = buffs.ship->GetModuleApplier().GetLegendaryPrimedArsenalRuntime().GetRules().front();
		buffs.ship->GetModuleApplier().Tick(buffRule.readyWindowSeconds);
		report.Check(drive.TryActivate(), "powered AP buff starts");
		const float buffPower = 1.f + buffRule.powerPerReadySecond * buffRule.readyWindowSeconds;
		buffs.Equal("AP buff amount scales once without owner feedback", baseBonus * buffPower, buffs.Owner(OwnerAttributeIds::AttackPower) - ap);
		AbilityExecutionContext context{ &buffs.ship->GetAbilitySystemComponent(), &drive.GetDefinition(), nullptr, &drive };
		buffs.Equal("Primed does not amplify secondary movement strength", move, sas::FindAttributeValue(AbilityActionAttributeResolver::ResolveAbilityAttributes(context), AbilityData::ExecutionDrive::Attribute::MoveSpeedBonus));

		Fixture speed{ report, application };
		auto& overdrive = Grant(speed, "Ability.Offense.OverdriveCore.Basic", sas::AbilitySlot::Ability4);
		const float originalSpeed = speed.Owner(OwnerAttributeIds::AttackSpeed);
		report.Check(overdrive.TryActivate(), "baseline AS buff starts");
		overdrive.Tick(2.f);
		const float speedBonus = speed.Owner(OwnerAttributeIds::AttackSpeed) - originalSpeed;
		report.Check(speedBonus > 0.f, "AS buff is measured after its rocket launch phase");
		Ready(overdrive); speed.Install(Primed);
		const auto speedRule = speed.ship->GetModuleApplier().GetLegendaryPrimedArsenalRuntime().GetRules().front();
		speed.ship->GetModuleApplier().Tick(speedRule.readyWindowSeconds);
		report.Check(overdrive.TryActivate(), "powered AS buff starts");
		overdrive.Tick(2.f);
		speed.Equal("AS buff amount scales once", speedBonus * (1.f + speedRule.powerPerReadySecond * speedRule.readyWindowSeconds), speed.Owner(OwnerAttributeIds::AttackSpeed) - originalSpeed);
	}

	void HardlightPools(Report& report, Application& application)
	{
		for (int level : { 1, 3, 5 })
		{
			Fixture f{ report, application };
			auto& health = f.ship->GetHealthComponent(); auto& shield = f.ship->GetShieldComponent();
			f.Equal("known Fighter Hpre", 250.f, health.GetMaxHealth()); f.Equal("known Fighter shield", 140.f, shield.GetMaxShield());
			health.ChangeHealth(-125.f); shield.ChangeShield(-105.f);
			const auto schematic = Schematic(level > 3 ? GameplayTags::Module::Zone::Hull : GameplayTags::Module::Zone::Nose);
			const ModuleRulesContext rules{ schematic, f.rules.lookup, f.rules.zoneTable, f.rules.sideFeatureLookup };
			const float delay = shield.GetRechargeDelay(), remainingDelay = shield.GetRechargeDelayRemaining();
			const auto id = Install(f, rules, Hardlight, level > 3 ? level - 2 : level);
			const float maximum = 140.f + (.50f + .04f * (level - 1)) * 250.f;
			f.Equal("Hardlight converts HP to one", 1.f, health.GetMaxHealth());
			f.Equal("L1 L3 L5 conversion capacity", maximum, shield.GetMaxShield());
			f.Equal("install carries HP fill ratio", .5f, health.GetHealth());
			f.Equal("install carries shield fill ratio", maximum * .25f, shield.GetShield());
			f.Equal("downstream owner MaxHealth reads one", 1.f, f.Owner(OwnerAttributeIds::MaxHealth));
			f.Equal("shield delay preserved", delay, shield.GetRechargeDelay()); f.Equal("running shield delay preserved", remainingDelay, shield.GetRechargeDelayRemaining());
			for (int i = 0; i < 20; ++i) { f.ship->GetModuleApplier().Tick(0.f); f.ship->RefreshMovementAttributesFromRuntime(); }
			f.Equal("recompute does not convert one HP again", maximum, shield.GetMaxShield());
			auto& ability = Grant(f, "Ability.Offense.CombatSentry.Basic", sas::AbilitySlot::Ability4);
			AbilityExecutionContext context{ &f.ship->GetAbilitySystemComponent(), &ability.GetDefinition(), nullptr, &ability };
			const auto* actor = AbilityData::FindAbilityActorDefinition("Actor.Ability.CombatSentry.Turret.Basic");
			report.Check(actor != nullptr, "shipped HP-scaled summon");
			const auto values = AbilityActionAttributeResolver::ResolveAttributes(context, nullptr, actor->attributes);
			const float hpScale = sas::FindAttributeValue(values, AbilityData::CombatSentry::Actor::Turret::OwnerMaxHealthScale);
			report.Check(ability.TryActivate(), "activate real HP-scaled summon under Hardlight");
			const auto sentries = Actors<CombatSentryActor>(f);
			report.Check(sentries.size() == 1 && sentries.front()->FindComponent<HealthComponent>(), "real summon has health");
			f.Equal("HP-scaled summon reads converted one", sas::FindAttributeValue(values, AbilityData::CombatSentry::Actor::Turret::MaxHealth) + hpScale, sentries.front()->FindComponent<HealthComponent>()->GetMaxHealth());
			auto& attributes = f.ship->GetAbilitySystemComponent().GetAttributes();
			const auto extra = attributes.AddModifier({ OwnerAttributeIds::MaxHealth, sas::AttributeModifierOperation::Add, 100.f });
			f.ship->GetModuleApplier().Tick(0.f);
			f.Equal("changed original health source recomputes Hpre", 140.f + (.50f + .04f * (level - 1)) * 350.f, shield.GetMaxShield());
			attributes.RemoveModifier(extra); f.ship->GetModuleApplier().Tick(0.f);
			f.Equal("removing health contribution is stable", maximum, shield.GetMaxShield());
			report.Check(f.loadout.Uninstall(id) && f.ship->GetModuleApplier().Apply(f.loadout, rules), "uninstall Hardlight");
			f.Equal("uninstall restores original HP capacity", 250.f, health.GetMaxHealth());
			f.Equal("uninstall carries HP ratio without healing", 125.f, health.GetHealth());
			f.Equal("uninstall restores shield capacity", 140.f, shield.GetMaxShield());
			f.Equal("uninstall carries shield ratio", 35.f, shield.GetShield());
		}
	}

	void HardlightWithShieldModules(Report& report, Application& application)
	{
		Fixture f{ report, application };
		f.Install("Module.Defense.PolarShield.Rare");
		auto& shield = f.ship->GetShieldComponent();
		const float original = shield.GetMaxShield();
		shield.ChangeShield(-original * .5f);
		const float front = shield.GetFrontShield() / (original * .5f), rear = shield.GetRearShield() / (original * .5f);
		const float delay = shield.GetRechargeDelayRemaining();
		const auto id = f.Install(Hardlight);
		const float fraction = f.ship->GetModuleApplier().GetLegendaryHardlightHullRuntime().GetRules().front().healthToShieldFraction;
		const float converted = original + 250.f * fraction;
		f.Equal("existing shield module capacity resolves before conversion", converted, shield.GetMaxShield());
		report.Check(shield.GetModuleBehavior().frontRearSplit, "Polar split remains installed with Hardlight");
		f.Equal("conversion carries independent front fill", converted * .5f * front, shield.GetFrontShield());
		f.Equal("conversion carries independent rear fill", converted * .5f * rear, shield.GetRearShield());
		f.Equal("conversion preserves running shield delay with Polar", delay, shield.GetRechargeDelayRemaining());
		f.Remove(id);
		f.Equal("removal keeps Polar capacity", original, shield.GetMaxShield());
		f.Equal("removal keeps front fill", original * .5f * front, shield.GetFrontShield());
		f.Equal("removal keeps rear fill", original * .5f * rear, shield.GetRearShield());

		Fixture empty{ report, application };
		report.Check(empty.ship->GetShipRuntime().SetBaseMaxShieldContribution("Test.Power.EmptyShield", -140.f), "remove original shield through its stat source");
		empty.Equal("empty pool starts at zero", 0.f, empty.ship->GetShieldComponent().GetMaxShield());
		empty.Install(Hardlight);
		empty.Equal("a missing shield pool cannot grant free shield on conversion", 0.f, empty.ship->GetShieldComponent().GetShield());
	}
}

int main(int argc, char** argv)
{
	vega::EngineContext context; vega::EngineContext::Scope scope{ context }; vega::InstallVegaEngineAssets();
	if (argc != 2) return 2;
	Report report;
	try
	{
		context.Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		report.Check(GameContentBootstrap::Register(), "register shipped game content");
		std::string title = "Legendary power modules E2E"; Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None }; application.GetRenderWindow().setVisible(false);
		PrimedLevels(report, application); ChannelAndReplay(report, application); ChargesAndIndependentReserves(report, application); SpecialOutputs(report, application); GroupAndBuff(report, application); HardlightPools(report, application); HardlightWithShieldModules(report, application);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& error) { report.artifact["error"] = error.what(); std::cerr << error.what() << '\n'; }
	report.artifact["test"] = "LightYearsLegendaryPowerModulesE2E"; WriteArtifact(argv[1], report.artifact); return report.artifact.value("passed", false) ? 0 : 1;
}
