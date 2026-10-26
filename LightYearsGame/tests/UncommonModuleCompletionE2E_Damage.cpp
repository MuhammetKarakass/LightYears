#include "UncommonModuleCompletionE2E.h"
#include "gameplay/content/DamageStatusBalanceCatalog.h"
namespace uncommon_completion_e2e
{
	void DamageChecks(Report& report, Application& application)
	{
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application };
			const float power = f.Owner(OwnerAttributeIds::EnergyPower);
			const auto id = f.Install("Module.Offense.SharpScale.Uncommon", 1, level, "Module.SideFeature.Shared.O5");
			f.Equal("O5 adds a flat +8 Energy Power at every side level", power + 8.f, f.Owner(OwnerAttributeIds::EnergyPower));
			auto target = Enemy(f);
			f.Equal("O5 gives no first-hit damage bonus", 10.f, Hit(*target, f.ship.get(), 10.f, Plain()));
			f.Equal("O5 gives no repeat-hit damage bonus", 10.f, Hit(*target, f.ship.get(), 10.f, Plain()));
			f.Remove(id);
			f.Equal("removing O5 releases its Energy Power", power, f.Owner(OwnerAttributeIds::EnergyPower));
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
