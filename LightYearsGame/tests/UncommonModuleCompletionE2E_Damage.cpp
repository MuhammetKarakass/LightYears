#include "UncommonModuleCompletionE2E.h"
#include "gameplay/content/DamageStatusBalanceCatalog.h"
namespace uncommon_completion_e2e
{
	namespace
	{
		struct NestedHit
		{
			SpaceShip& target; Actor& source; bool ran = false; float damage = 0.f; bool beforeApply; DelegateHandle handle;
			NestedHit(SpaceShip& value, Actor& attacker, bool before = false) : target{ value }, source{ attacker }, beforeApply{ before }, handle{ (before ? target.GetCombatRuntime().onDamageProcessed : target.GetCombatRuntime().onDamageResolved).BindAction(this, &NestedHit::Resolved) } {}
			~NestedHit() { (beforeApply ? target.GetCombatRuntime().onDamageProcessed : target.GetCombatRuntime().onDamageResolved).UnbindAction(handle); }
			void Resolved(const DamageContext&) { if (!ran) { ran = true; damage = Hit(target, &source, 10.f, Plain()); } }
		};
		struct RetainedFailedHit
		{
			SpaceShip& target; DamageContext snapshot; bool ran = false; DelegateHandle handle;
			explicit RetainedFailedHit(SpaceShip& value) : target{ value }, handle{ target.GetCombatRuntime().onDamageProcessed.BindAction(this, &RetainedFailedHit::Processed) } {}
			~RetainedFailedHit() { target.GetCombatRuntime().onDamageProcessed.UnbindAction(handle); }
			void Processed(const DamageContext& context) { if (!ran) { ran = true; snapshot = context; throw std::runtime_error{ "E2E interrupted before application" }; } }
		};
	}
	void DamageChecks(Report& report, Application& application)
	{
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application };
			const auto id = f.Install("Module.Offense.SharpScale.Uncommon", 1, level, "Module.SideFeature.Shared.O5");
			auto target = Enemy(f);
			target->SetInvulnerability(true);
			f.Equal("blocked hit applies no damage", 0.f, Hit(*target, f.ship.get(), 10.f, Plain()));
			report.Check(f.ship->GetCombatRuntime().GetModuleFirstHitTargetCount() == 0, "blocked first hit rolls back its reservation");
			target->SetInvulnerability(false);
			f.Equal("first successful target hit receives O5", 11.5f, Hit(*target, f.ship.get(), 10.f, Plain()));
			f.Equal("later hit receives no O5", 10.f, Hit(*target, f.ship.get(), 10.f, Plain()));
			const auto unrelated = f.Install("Module.Utility.DoubleFeedCore.Uncommon");
			f.Equal("unrelated rebuild retains first-hit history", 10.f, Hit(*target, f.ship.get(), 10.f, Plain()));
			f.Remove(unrelated);
			f.Remove(id);
			f.Install("Module.Offense.SharpScale.Uncommon", 1, level, "Module.SideFeature.Shared.O5");
			f.Equal("reequip cannot farm O5 on an existing target", 10.f, Hit(*target, f.ship.get(), 10.f, Plain()));
			auto fresh = Enemy(f, 200.f);
			{ NestedHit observer{ *fresh, *f.ship }; Hit(*fresh, f.ship.get(), 10.f, Plain()); f.Equal("nested resolved hit cannot repeat O5", 10.f, observer.damage); }
			auto inflight = Enemy(f, 250.f);
			{ NestedHit observer{ *inflight, *f.ship, true }; Hit(*inflight, f.ship.get(), 10.f, Plain()); f.Equal("in-flight reservation prevents nested O5", 10.f, observer.damage); }
			auto skipped = Enemy(f, 300.f);
			auto payload = Plain(); payload.skipModuleDamageRules = true;
			f.Equal("generated hit receives no O5", 10.f, Hit(*skipped, f.ship.get(), 10.f, payload));
			payload.skipModuleDamageRules = false; payload.moduleTriggerPolicy = DamageModuleTriggerPolicy::Suppressed;
			f.Equal("suppressed hit receives no O5", 10.f, Hit(*skipped, f.ship.get(), 10.f, payload));
			f.Equal("generated and suppressed hits do not consume O5", 11.5f, Hit(*skipped, f.ship.get(), 10.f, Plain()));
			auto interrupted = Enemy(f, 350.f);
			{ RetainedFailedHit observer{ *interrupted }; bool rejected = false; try { Hit(*interrupted, f.ship.get(), 10.f, Plain()); } catch (const std::runtime_error&) { rejected = true; }
				report.Check(rejected && observer.snapshot.moduleFirstHitClaim.expired(), "retained interrupted damage snapshot cannot keep a reservation alive");
				f.Equal("interrupted application leaves O5 for the next successful hit", 11.5f, Hit(*interrupted, f.ship.get(), 10.f, Plain())); }
			const auto count = f.ship->GetCombatRuntime().GetModuleFirstHitTargetCount();
			target->Destroy(); f.level->TickInternal(0.f); target.reset();
			report.Check(f.ship->GetCombatRuntime().GetModuleFirstHitTargetCount() == count - 1, "first-hit weak history prunes destroyed target");
		}
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application };
			const auto id = f.Install("Module.Offense.ScatterCore.Uncommon", 1, level, "Module.SideFeature.Shared.O4");
			const auto& balance = content::DamageStatusBalanceCatalog::Get();
			const GameplayTag types[]{ DamageTypeSchema::Thermal, DamageTypeSchema::Cryo, DamageTypeSchema::Electric, DamageTypeSchema::Kinetic };
			const char* ids[]{ DamageStatusEffectIds::IgniteEffectId, DamageStatusEffectIds::CryoSlowedEffectId, DamageStatusEffectIds::ElectricEffectId, DamageStatusEffectIds::KineticEffectId };
			const float bases[]{ balance.thermal.duration, balance.cryo.duration, balance.electric.duration, balance.kinetic.duration };
			for (int i = 0; i < 4; ++i)
			{
				auto target = Enemy(f, 400.f + i * 100.f);
				auto payload = Plain(); payload.igniteStacks = payload.cryoBuildupPerHit = payload.electricStacks = payload.kineticStacks = 20;
				Hit(*target, f.ship.get(), .1f, payload, {}, {}, DamageDeliveryType::Direct, { types[i] });
				report.Check(Effect(*target, ids[i]) != nullptr, "O4 applies real canonical status");
				f.Equal("O4 adds one second to status duration", bases[i] + 1.f, Effect(*target, ids[i])->spec.duration);
				Hit(*target, f.ship.get(), .1f, payload, {}, {}, DamageDeliveryType::Direct, { types[i] });
				f.Equal("capped reapply does not accumulate duration", bases[i] + 1.f, Effect(*target, ids[i])->spec.duration);
				payload.moduleTriggerPolicy = DamageModuleTriggerPolicy::Suppressed;
				Hit(*target, f.ship.get(), .1f, payload, {}, {}, DamageDeliveryType::Direct, { types[i] });
				f.Equal("suppressed status uses base duration", bases[i], Effect(*target, ids[i])->spec.duration);
				payload.moduleTriggerPolicy = DamageModuleTriggerPolicy::Normal;
				Hit(*target, f.ship.get(), .1f, payload, {}, {}, DamageDeliveryType::Direct, { types[i] });
				f.Equal("normal reapply restores exactly one bonus", bases[i] + 1.f, Effect(*target, ids[i])->spec.duration);
				auto other = Enemy(f, 1000.f);
				Hit(*target, other.get(), .1f, payload, {}, {}, DamageDeliveryType::Direct, { types[i] });
				f.Equal("new source replaces duration with its own base", bases[i], Effect(*target, ids[i])->spec.duration);
			}
			f.Install("Module.Utility.FrostLock.Uncommon", 0, level);
			auto frozen = Enemy(f, 2000.f); StatusHit(*frozen, f.ship.get(), DamageTypeSchema::Cryo, 4);
			f.Equal("O4 extends applied Frozen", 2.f + .3f * (level - 1), Effect(*frozen, DamageStatusEffectIds::CryoFrozenEffectId)->spec.duration);
			f.Equal("O4 leaves freeze cooldown unchanged", 8.f - (level - 1), Effect(*frozen, DamageStatusEffectIds::CryoFreezeCooldownEffectId)->spec.duration);
			f.Remove(id);
			auto base = Enemy(f); StatusHit(*base, f.ship.get(), DamageTypeSchema::Electric);
			f.Equal("removing O4 clears duration bonus", balance.electric.duration, Effect(*base, DamageStatusEffectIds::ElectricEffectId)->spec.duration);
		}
	}
}
