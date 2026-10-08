#include "UncommonModuleP5E2E.h"
#include "gameplay/targeting/CombatantTargetQuery.h"
#include "gameplay/movement/MovementInfluenceService.h"

namespace uncommon_p5_e2e
{
	namespace
	{
		struct ElectricCallbackObserver
		{
			SpaceShip& target;
			DelegateHandle handle;
			int lastStack = 0;
			float lastBonus = 0.f;
			explicit ElectricCallbackObserver(SpaceShip& value) : target{ value }, handle{ value.GetAbilitySystemComponent().onGameplayEffectChanged.BindAction(this, &ElectricCallbackObserver::Changed) } {}
			~ElectricCallbackObserver() { target.GetAbilitySystemComponent().onGameplayEffectChanged.UnbindAction(handle); }
			void Changed(sas::GameplayEffectHandle changed)
			{
				const auto* effect = Effect(target, DamageStatusEffectIds::ElectricEffectId);
				if (!effect || effect->handle != changed) return;
				lastStack = effect->stackCount;
				const auto* bonus = sas::FindAttribute(effect->spec.attributes, DamageAttributeIds::ElectricDamageTakenBonusForStack(lastStack));
				lastBonus = bonus ? bonus->currentValue : 0.f;
			}
		};
	}
	void StatusChecks(Report& report, Application& application)
	{
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application };
			const auto baseHandles = f.ship->GetModuleApplier().GetOwnedHandleCount();
			const auto arc = f.Install("Module.Offense.ArcExtender.Uncommon", 0, level);
			const auto enemy = Enemy(f);
			ElectricCallbackObserver callback{ *enemy };
			const float bonus[] = { 0.03f, 0.06f, 0.09f, 0.13f, 0.17f + level * 0.02f };
			for (int stack = 1; stack <= 5; ++stack)
			{
				StatusHit(*enemy, f.ship.get(), DamageTypeSchema::Electric);
				report.Check(Stacks(*enemy, DamageStatusEffectIds::ElectricEffectId) == stack, "Arc stacks through the real damage path", { level, stack });
				f.Equal("Arc stack-specific damage bonus", 10.f * (1.f + bonus[stack - 1]), Hit(*enemy, f.ship.get(), 10.f, Plain()));
			}
			StatusHit(*enemy, f.ship.get(), DamageTypeSchema::Electric, 20);
			report.Check(Stacks(*enemy, DamageStatusEffectIds::ElectricEffectId) == 5, "Arc has a technical five-stack cap");
			report.Check(callback.lastStack == 5, "P5 observes Electric through a real effect-change callback");
			f.Equal("effect-change callbacks see a coherent applied Electric profile", bonus[4], callback.lastBonus);
			enemy->GetAbilitySystemComponent().Tick(5.5f);
			StatusHit(*enemy, f.ship.get(), DamageTypeSchema::Electric, 0);
			f.Equal("Arc zero-stack same-type hit refreshes duration", 6.f, Effect(*enemy, DamageStatusEffectIds::ElectricEffectId)->remainingDuration);
			enemy->GetAbilitySystemComponent().Tick(6.01f);
			report.Check(Stacks(*enemy, DamageStatusEffectIds::ElectricEffectId) == 0, "Arc expires all stacks after the unchanged duration");
			StatusHit(*enemy, f.ship.get(), DamageTypeSchema::Electric, 5);
			report.Check(Stacks(*enemy, DamageStatusEffectIds::ElectricEffectId) == 5, "Arc accepts five stacks in one hit");
			sas::GameplayAttributeList attributes{ { DamageAttributeIds::ElectricStacks, 5.f, 5.f } };
			report.Check(DamageTypeSystem::BuildPayload({ DamageTypeSchema::Electric }, attributes).electricStacks == 5, "generic payload preserves the source's fifth stack for owner-specific resolution");
			const auto other = Enemy(f, 900.f);
			enemy->GetAbilitySystemComponent().Tick(0.5f);
			StatusHit(*enemy, other.get(), DamageTypeSchema::Electric, 0);
			f.Equal("another source's zero-stack hit cannot refresh duration", 5.5f, Effect(*enemy, DamageStatusEffectIds::ElectricEffectId)->remainingDuration);
			f.Equal("another source's zero-stack hit cannot replace the applied profile", 10.f * (1.f + bonus[4]), Hit(*enemy, f.ship.get(), 10.f, Plain()));
			StatusHit(*enemy, other.get(), DamageTypeSchema::Electric, 1);
			report.Check(Stacks(*enemy, DamageStatusEffectIds::ElectricEffectId) == 4, "another source keeps its own global cap on a shared target");
			f.Equal("another source uses the global fourth-stack bonus", 11.6f, Hit(*enemy, f.ship.get(), 10.f, Plain()));
			StatusHit(*enemy, f.ship.get(), DamageTypeSchema::Electric);
			report.Check(Stacks(*enemy, DamageStatusEffectIds::ElectricEffectId) == 5, "Arc can reclaim its profile from an ordinary source");
			f.Remove(arc);
			StatusHit(*enemy, f.ship.get(), DamageTypeSchema::Electric);
			report.Check(Stacks(*enemy, DamageStatusEffectIds::ElectricEffectId) == 4, "Arc removal restores the global cap on next application");
			f.Equal("Arc removal restores the global damage table", 11.6f, Hit(*enemy, f.ship.get(), 10.f, Plain()));
			report.Check(f.ship->GetModuleApplier().GetOwnedHandleCount() == baseHandles && f.ship->GetCombatRuntime().GetModuleStatusRules().Empty(), "Arc leaves no module handles or status rules after removal");
		}
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application };
			const auto frost = f.Install("Module.Utility.FrostLock.Uncommon", 0, level);
			const auto enemy = Enemy(f);
			for (int stack = 1; stack <= 3; ++stack) StatusHit(*enemy, f.ship.get(), DamageTypeSchema::Cryo);
			report.Check(!Effect(*enemy, DamageStatusEffectIds::CryoFrozenEffectId), "Frost excludes stacks below threshold");
			StatusHit(*enemy, f.ship.get(), DamageTypeSchema::Cryo);
			report.Check(Effect(*enemy, DamageStatusEffectIds::CryoFrozenEffectId) != nullptr, "Frost freezes at four stacks");
			f.Equal("Frost freeze duration follows level table", 0.7f + level * 0.3f, Effect(*enemy, DamageStatusEffectIds::CryoFrozenEffectId)->remainingDuration);
			f.Equal("Frost target cooldown follows level table", 9.f - level, Effect(*enemy, DamageStatusEffectIds::CryoFreezeCooldownEffectId)->remainingDuration);
			report.Check(Stacks(*enemy, DamageStatusEffectIds::CryoSlowedEffectId) == 0, "Frost clears Cryo stacks");
			f.Equal("Frost removes the Cryo slow modifier", 0.f, enemy->GetAbilitySystemComponent().GetAttributes().GetCurrentValue(OwnerAttributeIds::MovementSlow));
			report.Check(enemy->GetAbilitySystemComponent().HasOwnedTag(GameplayTags::State::Effect::Control::Stunned), "Frost uses the canonical movement and attack stun lock");
			report.Check(enemy->GetAbilitySystemComponent().HasOwnedTag(GameplayTags::Status::Damage::Cryo::Frozen), "Frost exposes the canonical frozen status to the movement component");
			enemy->SetVelocity({ 120.f, 0.f });
			const auto location = enemy->GetActorLocation();
			enemy->GetMovementComponent().Tick(0.1f);
			report.Check(enemy->GetVelocity() == sf::Vector2f{} && enemy->GetActorLocation() == location, "Frost stops an already-moving ship through the real movement component");
			report.Check(movement::MovementInfluenceService::SetForcedMovement(*enemy, { f.ship->GetUniqueID(), { 300.f, 0.f }, 1 }), "Frost scenario uses the real external movement service");
			enemy->GetMovementComponent().Tick(0.1f);
			report.Check(enemy->GetActorLocation() == location, "Frost blocks forced movement while frozen");
			movement::MovementInfluenceService::RemoveSource(*enemy, f.ship->GetUniqueID());
			const auto* primaryDefinition = f.ship->GetAbilitySystemComponent().GetAbility(sas::AbilitySlot::PrimaryFire);
			report.Check(primaryDefinition != nullptr, "Frost test uses the shipped primary ability");
			auto& enemyAbilities = enemy->GetAbilitySystemComponent();
			if (!enemyAbilities.GetAbility(sas::AbilitySlot::PrimaryFire))
				report.Check(enemyAbilities.GrantAbility(primaryDefinition->GetDefinition()).IsValid(), "Frost grants the real primary weapon to the target");
			auto* primary = enemyAbilities.GetAbility(sas::AbilitySlot::PrimaryFire);
			report.Check(primary && !primary->TryActivate(), "Frost blocks actual primary attack activation");
			StatusHit(*enemy, f.ship.get(), DamageTypeSchema::Cryo, 4);
			enemy->GetAbilitySystemComponent().Tick(0.71f + level * 0.3f);
			report.Check(!Effect(*enemy, DamageStatusEffectIds::CryoFrozenEffectId) && Effect(*enemy, DamageStatusEffectIds::CryoFreezeCooldownEffectId), "freeze ends before its per-target cooldown");
			enemy->SetVelocity({ 120.f, 0.f });
			enemy->GetMovementComponent().Tick(0.1f);
			report.Check(enemy->GetActorLocation() != location, "movement resumes after freeze expiry");
			report.Check(primary->TryActivate(), "actual primary attack resumes after freeze expiry");
			if (primary->IsActive()) primary->Cancel(sas::AbilityEndReason::Completed);
			StatusHit(*enemy, f.ship.get(), DamageTypeSchema::Cryo, 4);
			report.Check(!Effect(*enemy, DamageStatusEffectIds::CryoFrozenEffectId), "hits during cooldown cannot freeze again");
			enemy->GetAbilitySystemComponent().Tick(9.f);
			primary->Tick(100.f);
			report.Check(primary->TryActivate() && primary->IsActive(), "Frost scenario starts with a running automatic primary attack");
			StatusHit(*enemy, f.ship.get(), DamageTypeSchema::Cryo, 4);
			report.Check(Effect(*enemy, DamageStatusEffectIds::CryoFrozenEffectId), "Frost can freeze after cooldown");
			enemyAbilities.Tick(0.f);
			report.Check(!primary->IsActive(), "Frost interrupts an already-running attack through the real ASC");
			for (const auto targetClass : { ControlTargetClass::Elite, ControlTargetClass::MiniBoss, ControlTargetClass::Boss })
			{
				const auto immune = Enemy(f, 900.f);
				immune->SetControlTargetClass(targetClass);
				StatusHit(*immune, f.ship.get(), DamageTypeSchema::Cryo, 4);
				report.Check(!Effect(*immune, DamageStatusEffectIds::CryoFrozenEffectId) && Stacks(*immune, DamageStatusEffectIds::CryoSlowedEffectId) == 4, "elite and boss classes retain ordinary Cryo without freezing", static_cast<int>(targetClass));
			}
			const auto suppressed = Enemy(f, 950.f);
			StatusHit(*suppressed, f.ship.get(), DamageTypeSchema::Cryo, 4, true);
			report.Check(!Effect(*suppressed, DamageStatusEffectIds::CryoFrozenEffectId), "suppressed module damage cannot trigger Frost");
			const auto generated = Enemy(f, 950.f);
			auto generatedPayload = Plain(); generatedPayload.skipModuleDamageRules = true; generatedPayload.cryoBuildupPerHit = 4;
			Hit(*generated, f.ship.get(), 0.1f, generatedPayload, {}, {}, DamageDeliveryType::Direct, { DamageTypeSchema::Cryo });
			report.Check(!Effect(*generated, DamageStatusEffectIds::CryoFrozenEffectId), "outgoing module bonus cannot trigger Frost through the legacy skip flag");
			const auto protectedTarget = Enemy(f, 980.f);
			sas::GameplayEffectDefinition immunity;
			immunity.effectId = "Effect.Test.P5.ControlImmunity";
			immunity.durationPolicy = sas::GameplayEffectDurationPolicy::Duration;
			immunity.duration = 10.f;
			immunity.grantedImmunityCategory = "Control";
			report.Check(protectedTarget->GetAbilitySystemComponent().ApplyGameplayEffect(sas::MakeGameplayEffectSpec(immunity)).IsValid(), "real ASC grants control immunity for the rejection scenario");
			StatusHit(*protectedTarget, f.ship.get(), DamageTypeSchema::Cryo, 4);
			report.Check(!Effect(*protectedTarget, DamageStatusEffectIds::CryoFrozenEffectId) && !Effect(*protectedTarget, DamageStatusEffectIds::CryoFreezeCooldownEffectId) && Stacks(*protectedTarget, DamageStatusEffectIds::CryoSlowedEffectId) == 4, "rejected freeze consumes neither stacks nor cooldown");
			f.Remove(frost);
			const auto ordinary = Enemy(f, 1000.f);
			StatusHit(*ordinary, f.ship.get(), DamageTypeSchema::Cryo, 8);
			report.Check(!Effect(*ordinary, DamageStatusEffectIds::CryoFrozenEffectId) && Stacks(*ordinary, DamageStatusEffectIds::CryoSlowedEffectId) == 4, "Frost removal restores ordinary four-stack Cryo");
		}
		for (int variant = 0; variant < 2; ++variant)
		{
			Fixture f{ report, application };
			const auto coil = f.Install("Module.Offense.ThermalShockCoil.Uncommon", variant, 1, "Module.SideFeature.ThermalShockCoil.LingeringShock");
			const auto enemy = Enemy(f);
			StatusHit(*enemy, f.ship.get(), DamageTypeSchema::Cryo);
			StatusHit(*enemy, f.ship.get(), DamageTypeSchema::Thermal);
			f.Equal("Lingering excludes the first hit creating the pair", 5.f, Effect(*enemy, DamageStatusEffectIds::IgniteEffectId)->spec.duration);
			StatusHit(*enemy, f.ship.get(), DamageTypeSchema::Thermal);
			StatusHit(*enemy, f.ship.get(), DamageTypeSchema::Cryo);
			f.Equal("Lingering Thermal respects largest-variant lock", variant == 1 ? 7.f : 5.f, Effect(*enemy, DamageStatusEffectIds::IgniteEffectId)->spec.duration);
			f.Equal("Lingering Cryo respects largest-variant lock", variant == 1 ? 8.f : 6.f, Effect(*enemy, DamageStatusEffectIds::CryoSlowedEffectId)->spec.duration);
			StatusHit(*enemy, f.ship.get(), DamageTypeSchema::Thermal);
			f.Equal("Lingering adds a fixed bonus without accumulating", variant == 1 ? 7.f : 5.f, Effect(*enemy, DamageStatusEffectIds::IgniteEffectId)->spec.duration);
			f.Remove(coil);
			StatusHit(*enemy, f.ship.get(), DamageTypeSchema::Thermal);
			f.Equal("Lingering removal restores new status duration", 5.f, Effect(*enemy, DamageStatusEffectIds::IgniteEffectId)->spec.duration);
		}
		for (int variant = 0; variant < 2; ++variant)
		{
			Fixture f{ report, application };
			f.Install("Module.Utility.FrostLock.Uncommon");
			const auto cold = f.Install("Module.Defense.ColdRetort.Uncommon", variant, 1, "Module.SideFeature.ColdRetort.FrozenSpread");
			const auto center = Enemy(f, 50.f);
			const auto near = Enemy(f, 449.f);
			const auto far = Enemy(f, 451.f);
			const auto chained = Enemy(f, 200.f);
			StatusHit(*chained, f.ship.get(), DamageTypeSchema::Cryo, 3);
			const float health = near->GetHealthComponent().GetHealth();
			StatusHit(*center, f.ship.get(), DamageTypeSchema::Cryo, 4);
			report.Check(Stacks(*near, DamageStatusEffectIds::CryoSlowedEffectId) == (variant == 1 ? 1 : 0), "Frozen Spread uses 400 units from the frozen target and respects variant lock");
			report.Check(Stacks(*far, DamageStatusEffectIds::CryoSlowedEffectId) == 0, "Frozen Spread excludes targets outside 400 units");
			report.Check(Stacks(*chained, DamageStatusEffectIds::CryoSlowedEffectId) == (variant == 1 ? 4 : 3) && !Effect(*chained, DamageStatusEffectIds::CryoFrozenEffectId), "spread is status only and cannot chain a second freeze");
			f.Equal("Frozen Spread deals no damage", health, near->GetHealthComponent().GetHealth());
			f.Remove(cold);
			const auto next = Enemy(f, 40.f);
			StatusHit(*next, f.ship.get(), DamageTypeSchema::Cryo, 4);
			report.Check(Stacks(*near, DamageStatusEffectIds::CryoSlowedEffectId) == (variant == 1 ? 1 : 0), "Frozen Spread removal stops subsequent propagation");
		}
		Fixture f{ report, application };
		const auto handles = f.ship->GetModuleApplier().GetOwnedHandleCount();
		const auto liveArc = f.Install("Module.Offense.ArcExtender.Uncommon");
		const auto liveEnemy = Enemy(f);
		StatusHit(*liveEnemy, f.ship.get(), DamageTypeSchema::Electric, 5);
		report.Check(f.loadout.SetLevel(f.rules, liveArc, 3) && f.ship->GetModuleApplier().Apply(f.loadout, f.rules), "P5 level mutation reapplies through the canonical loadout revision");
		StatusHit(*liveEnemy, f.ship.get(), DamageTypeSchema::Electric, 0);
		f.Equal("P5 live level change replaces the fifth-stack profile", 12.3f, Hit(*liveEnemy, f.ship.get(), 10.f, Plain()));
		const auto arcPlacement = *f.loadout.FindPlacement(liveArc);
		f.Remove(liveArc);
		report.Check(f.loadout.Install(f.rules, liveArc, arcPlacement.origin, arcPlacement.rotationQuarterTurns) == ModulePlacementFailure::None && f.ship->GetModuleApplier().Apply(f.loadout, f.rules, true), "P5 combat install uses the real reboot path");
		report.Check(f.ship->GetModuleApplier().IsRebooting(liveArc) && f.ship->GetCombatRuntime().GetModuleStatusRules().Empty(), "P5 status rules stay inactive during reboot");
		StatusHit(*liveEnemy, f.ship.get(), DamageTypeSchema::Electric, 5);
		report.Check(Stacks(*liveEnemy, DamageStatusEffectIds::ElectricEffectId) == 4, "rebooting Arc keeps the global four-stack cap");
		f.ship->GetModuleApplier().Tick(f.rules.zoneTable.rebootDurationSeconds + 0.01f);
		StatusHit(*liveEnemy, f.ship.get(), DamageTypeSchema::Electric);
		report.Check(!f.ship->GetModuleApplier().IsRebooting(liveArc) && Stacks(*liveEnemy, DamageStatusEffectIds::ElectricEffectId) == 5, "P5 reboot expiry installs the resolved status rule");
		f.Remove(liveArc);
		for (int cycle = 0; cycle < 10; ++cycle)
		{
			const auto arc = f.Install("Module.Offense.ArcExtender.Uncommon");
			const auto frost = f.Install("Module.Utility.FrostLock.Uncommon");
			report.Check(f.ship->GetCombatRuntime().GetModuleStatusRules().electricOverrides.size() == 1 && f.ship->GetCombatRuntime().GetModuleStatusRules().cryoFreezes.size() == 1, "ten-cycle apply keeps one copy of each status rule", cycle);
			f.Remove(arc); f.Remove(frost);
			report.Check(f.ship->GetCombatRuntime().GetModuleStatusRules().Empty() && f.ship->GetModuleApplier().GetOwnedHandleCount() == handles, "ten-cycle removal leaves stable handles and no rules", cycle);
		}
	}
}
