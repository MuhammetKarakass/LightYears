#include "UncommonModuleP4E2E.h"
#include "gameplay/targeting/CombatantTargetQuery.h"

namespace uncommon_p4_e2e
{
	namespace
	{
		int Cryo(const SpaceShip& target)
		{
			const auto* effect = target.GetAbilitySystemComponent().FindGameplayEffectById(DamageStatusEffectIds::CryoSlowedEffectId);
			return effect ? effect->stackCount : 0;
		}
		void DefenseCast(Fixture& f)
		{
			auto* ability = f.ship->GetAbilitySystemComponent().GetAbility(sas::AbilitySlot::Ability4);
			ability->Tick(100.f);
			f.report.Check(ability->TryActivate(), "Reflection window begins through real Defense activation");
			if (ability->IsActive()) ability->Cancel(sas::AbilityEndReason::Completed);
			f.ship->SetInvulnerability(false);
		}
		struct DamageObserver
		{
			CombatRuntime& runtime;
			DelegateHandle handle;
			bool critical = false;
			bool generated = false;
			bool suppressed = false;
			bool criticalDisabled = false;
			float amount = 0.f;
			explicit DamageObserver(CombatRuntime& value) : runtime{ value }, handle{ runtime.onDamageResolved.BindAction(this, &DamageObserver::OnDamage) } {}
			~DamageObserver() { runtime.onDamageResolved.UnbindAction(handle); }
			void OnDamage(const DamageContext& context) { critical = context.wasCritical; generated = context.payload.skipModuleDamageRules; suppressed = context.payload.moduleTriggerPolicy == DamageModuleTriggerPolicy::Suppressed; criticalDisabled = context.payload.criticalPolicy == DamageCriticalPolicy::Disabled; amount = context.originalDamage; }
		};
	}
	void ReactionChecks(Report& report, Application& application)
	{
		for (int variant = 0; variant < 2; ++variant)
			for (int level = 1; level <= 3; ++level)
			{
				Fixture f{ report, application };
				List<shared_ptr<SpaceShip>> enemies;
				for (float distance : { 50.f, 100.f, 150.f, 200.f, 350.f }) enemies.push_back(Enemy(f, distance));
				const auto cold = f.Install("Module.Defense.ColdRetort.Uncommon", variant, level);
				const auto targets = targeting::FindOpposingCombatants(*f.level, *f.ship, 300.f, false);
				report.Check(targets.size() == 4, "Cold Retort scene exposes four nearby opposing combatants", { targets.size(), static_cast<int>(f.ship->GetCollisionLayer()) });
				ApplyCombatDamage(*f.ship, 1.f, enemies.front().get(), {}, Plain());
				for (int index = 0; index < 5; ++index) report.Check(Cryo(*enemies[index]) == (index < level + 1 ? 1 : 0), "Cold Retort selects nearest targets by level and range", { index, Cryo(*enemies[index]) });
				const float health = enemies.front()->GetHealthComponent().GetHealth();
				ApplyCombatDamage(*f.ship, 1.f, enemies.front().get(), {}, Plain());
				report.Check(Cryo(*enemies.front()) == 1, "Cold Retort has a two-second cooldown");
				f.ship->GetModuleApplier().Tick(2.01f);
				ApplyCombatDamage(*f.ship, 1.f, enemies.front().get(), {}, Plain());
				report.Check(Cryo(*enemies.front()) == 2, "Cold Retort becomes eligible after cooldown");
				f.Equal("Cold Retort status application deals no damage", health, enemies.front()->GetHealthComponent().GetHealth());
				f.ship->GetModuleApplier().Tick(2.01f);
				DamagePayload generated = Plain(); generated.skipModuleDamageRules = true;
				ApplyCombatDamage(*f.ship, 1.f, enemies.front().get(), {}, generated);
				report.Check(Cryo(*enemies.front()) == 2, "module-generated damage cannot trigger Cold Retort");
				f.Remove(cold);
				ApplyCombatDamage(*f.ship, 1.f, enemies.front().get(), {}, Plain());
				report.Check(Cryo(*enemies.front()) == 2, "Cold Retort removal clears future reactions");
			}
		for (int variant = 0; variant < 2; ++variant)
		{
			Fixture f{ report, application };
			const auto enemy = Enemy(f);
			f.ship->GetShieldComponent().ChangeShield(-10000.f);
			f.ship->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Armor, 0.f);
			const auto cold = f.Install("Module.Defense.ColdRetort.Uncommon", variant, 1, "Module.SideFeature.ColdRetort.JarringEffect");
			f.Equal("Jarring excludes a source without status", 10.f, Hit(*f.ship, enemy.get(), 10.f, Plain()));
			f.Equal("Jarring reads status on the source after Cold Retort", variant == 1 ? 8.8f : 10.f, Hit(*f.ship, enemy.get(), 10.f, Plain()));
			f.Remove(cold);
			f.Equal("removing Jarring restores incoming damage", 10.f, Hit(*f.ship, enemy.get(), 10.f, Plain()));
		}
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application };
			const auto nearest = Enemy(f, 50.f);
			const auto far = Enemy(f, 100.f);
			DamageObserver observer{ nearest->GetCombatRuntime() };
			const auto reflection = f.Install("Module.Defense.ReflectionShield.Uncommon", 0, level);
			const auto feedback = f.Install("Module.Offense.EnergyFeedback.Basic");
			const float power = f.Owner(OwnerAttributeIds::EnergyPower);
			f.ship->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::CriticalChance, 100.f);
			ApplyCombatDamage(*f.ship, 10.f, far.get(), {}, Plain());
			f.Equal("Reflection is inactive before Defense use", 1000.f, nearest->GetHealthComponent().GetHealth());
			DefenseCast(f);
			f.ship->GetShieldComponent().ChangeShield(10000.f);
			ApplyCombatDamage(*f.ship, 10.f, far.get(), {}, Plain());
			const float reflected = 10.f * (.35f + .05f * (level - 1));
			f.Equal("Reflection uses absorbed shield damage and level", reflected, observer.amount);
			report.Check(!observer.critical && observer.criticalDisabled && observer.generated && observer.suppressed, "Reflection disables critical hits and carries chain guard");
			f.Equal("Reflection picks nearest enemy instead of attacker", 1000.f, far->GetHealthComponent().GetHealth());
			f.Equal("Reflection cannot trigger Energy Feedback", power, f.Owner(OwnerAttributeIds::EnergyPower));
			f.ship->GetModuleApplier().Tick(4.f);
			DefenseCast(f);
			f.ship->GetModuleApplier().Tick(2.f);
			const float before = nearest->GetHealthComponent().GetHealth();
			ApplyCombatDamage(*f.ship, 10.f, far.get(), {}, Plain());
			f.Equal("Defense reuse refreshes the window", reflected, before - nearest->GetHealthComponent().GetHealth());
			f.ship->GetModuleApplier().Tick(3.01f);
			const float expired = nearest->GetHealthComponent().GetHealth();
			ApplyCombatDamage(*f.ship, 10.f, far.get(), {}, Plain());
			f.Equal("Reflection window expires", expired, nearest->GetHealthComponent().GetHealth());
			DefenseCast(f);
			f.ship->GetShieldComponent().ChangeShield(-10000.f);
			ApplyCombatDamage(*f.ship, 10.f, far.get(), {}, Plain());
			f.Equal("Reflection needs actual shield absorption", expired, nearest->GetHealthComponent().GetHealth());
			f.ship->GetShieldComponent().ChangeShield(10000.f);
			DamagePayload generated = Plain(); generated.skipModuleDamageRules = true;
			ApplyCombatDamage(*f.ship, 10.f, far.get(), {}, generated);
			f.Equal("Reflection cannot reflect module-generated hits", expired, nearest->GetHealthComponent().GetHealth());
			DamagePayload suppressed = Plain(); suppressed.moduleTriggerPolicy = DamageModuleTriggerPolicy::Suppressed;
			ApplyCombatDamage(*f.ship, 10.f, far.get(), {}, suppressed);
			f.Equal("explicit trigger suppression independently prevents reflection", expired, nearest->GetHealthComponent().GetHealth());
			f.Remove(reflection);
			ApplyCombatDamage(*f.ship, 10.f, far.get(), {}, Plain());
			f.Equal("active Reflection removes cleanly", expired, nearest->GetHealthComponent().GetHealth());
			f.Remove(feedback);
		}
		Fixture f{ report, application };
		const auto target = Enemy(f);
		const auto shatter = f.Install("Module.Defense.ShatterCurrent.Basic");
		f.ship->GetShieldComponent().ChangeShield(10000.f);
		DamagePayload generated = Plain(); generated.skipModuleDamageRules = true;
		generated.moduleTriggerPolicy = DamageModuleTriggerPolicy::Suppressed;
		ApplyCombatDamage(*f.ship, f.ship->GetShieldComponent().GetShield() + 1.f, target.get(), {}, generated);
		report.Check(f.ship->GetShieldComponent().GetShield() == 0.f, "generated damage really breaks the player's shield");
		report.Check(f.ship->GetCombatRuntime().ResolveOutgoingDamagePlan(*target, { GameplayTags::Ability::Primary }).bonuses.empty(), "generated shield break cannot open Shatter Current");
		f.ship->GetShieldComponent().ChangeShield(10000.f);
		ApplyCombatDamage(*f.ship, f.ship->GetShieldComponent().GetShield() + 1.f, target.get(), {}, Plain());
		report.Check(!f.ship->GetCombatRuntime().ResolveOutgoingDamagePlan(*target, { GameplayTags::Ability::Primary }).bonuses.empty(), "ordinary shield break still opens the Common damage window");
		f.Remove(shatter);
	}
}
