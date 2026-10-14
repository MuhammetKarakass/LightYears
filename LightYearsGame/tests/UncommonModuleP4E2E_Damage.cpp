#include "UncommonModuleP4E2E.h"

namespace uncommon_p4_e2e
{
	void DamageChecks(Report& report, Application& application)
	{
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application };
			const auto enemy = Enemy(f);
			f.Equal("no module preserves plain damage", 10.f, Hit(*enemy, f.ship.get(), 10.f, Plain()));
			const auto lens = f.Install("Module.Primary.PhotonicLens.Uncommon", 0, level);
			const float multiplier = 1.5f + .03f * (level - 1);
			f.Equal("Photonic Lens excludes non-primary direct damage", 10.f, Hit(*enemy, f.ship.get(), 10.f, Plain()));
			f.Equal("Photonic Lens excludes periodic damage", 10.f, Hit(*enemy, f.ship.get(), 10.f, Plain(), {}, {}, DamageDeliveryType::Periodic));
			DamagePayload cryo = Plain(); cryo.cryoBuildupPerHit = 1;
			f.Equal("Lens checks before the Primary hit applies its first status", 10.f * multiplier, Hit(*enemy, f.ship.get(), 10.f, cryo, {}, { GameplayTags::Ability::Primary }, DamageDeliveryType::Projectile, { DamageTypeSchema::Cryo }));
			report.Check(enemy->GetAbilitySystemComponent().HasOwnedTag(DamageStatusSchema::CryoSlowed), "real Cryo status was applied");
			f.Equal("Lens excludes a target carrying Cryo", 10.f, Hit(*enemy, f.ship.get(), 10.f, Plain()));
			const auto* status = enemy->GetAbilitySystemComponent().FindGameplayEffectById(DamageStatusEffectIds::CryoSlowedEffectId);
			enemy->GetAbilitySystemComponent().RemoveGameplayEffect(status->handle);
			enemy->GetAbilitySystemComponent().AddOwnedTag(GameplayTags::Status::Damage::Electric);
			f.Equal("Lens excludes other status families", 10.f, Hit(*enemy, f.ship.get(), 10.f, Plain()));
			f.Remove(lens);
			f.Equal("removing Lens restores plain damage", 10.f, Hit(*enemy, f.ship.get(), 10.f, Plain()));
		}
		for (int variant = 0; variant < 2; ++variant)
			for (int level = 1; level <= 3; ++level)
			{
				Fixture f{ report, application };
				const auto enemy = Enemy(f);
				const auto coil = f.Install("Module.Offense.ThermalShockCoil.Uncommon", variant, level);
				DamagePayload thermal = Plain(); thermal.igniteStacks = 1;
				DamagePayload cryo = Plain(); cryo.cryoBuildupPerHit = 1;
				f.Equal("Coil excludes targets without both statuses", 10.f, Hit(*enemy, f.ship.get(), 10.f, thermal, {}, {}, DamageDeliveryType::Direct, { DamageTypeSchema::Thermal }));
				f.Equal("Coil reads Cryo before this hit adds its first stack", 10.f, Hit(*enemy, f.ship.get(), 10.f, cryo, {}, {}, DamageDeliveryType::Direct, { DamageTypeSchema::Cryo }));
				const float multiplier = 1.3f + .05f * (level - 1);
				f.Equal("Coil resolves every size and level against real stacks", 10.f * multiplier, Hit(*enemy, f.ship.get(), 10.f, Plain()));
				f.Equal("Coil applies its all-damage bonus to periodic damage", 10.f * multiplier, Hit(*enemy, f.ship.get(), 10.f, Plain(), {}, {}, DamageDeliveryType::Periodic));
				DamagePayload generated = Plain(); generated.skipModuleDamageRules = true;
				f.Equal("generated damage bypasses outgoing Coil rules", 10.f, Hit(*enemy, f.ship.get(), 10.f, generated));
				f.Remove(coil);
				f.Equal("Coil removes cleanly", 10.f, Hit(*enemy, f.ship.get(), 10.f, Plain()));
			}
		Fixture f{ report, application };
		const auto enemy = Enemy(f);
		const float speed = f.Owner(OwnerAttributeIds::AttackSpeed);
		for (int variant = 0; variant < 2; ++variant)
		{
			const auto rhythm = f.Install("Module.Offense.SharpScale.Uncommon", variant, 1, "Module.SideFeature.SharpScale.CriticalRhythm");
			DamagePayload critical = Plain(); critical.criticalPolicy = DamageCriticalPolicy::Guaranteed;
			Hit(*enemy, f.ship.get(), 10.f, critical);
			f.Equal("Critical Rhythm respects largest-size unlock", speed + (variant == 1 ? 15.f : 0.f), f.Owner(OwnerAttributeIds::AttackSpeed));
			Hit(*enemy, f.ship.get(), 10.f, critical);
			f.Equal("critical-hit refresh does not stack", speed + (variant == 1 ? 15.f : 0.f), f.Owner(OwnerAttributeIds::AttackSpeed));
			f.ship->GetModuleTriggerRuntime().Tick(2.01f);
			f.Equal("Critical Rhythm expires after two seconds", speed, f.Owner(OwnerAttributeIds::AttackSpeed));
			critical.skipModuleDamageRules = true;
			Hit(*enemy, f.ship.get(), 10.f, critical);
			f.Equal("generated critical damage cannot grant Critical Rhythm", speed, f.Owner(OwnerAttributeIds::AttackSpeed));
			critical.skipModuleDamageRules = false;
			Hit(*enemy, f.ship.get(), 10.f, critical);
			f.Remove(rhythm);
			f.Equal("removing an active Critical Rhythm clears its modifier", speed, f.Owner(OwnerAttributeIds::AttackSpeed));
		}
		for (int variant = 0; variant < 2; ++variant)
		{
			const auto lens = f.Install("Module.Offense.SharpScale.Uncommon", variant, 1, "Module.SideFeature.SharpScale.LuckLens");
			auto& attributes = f.ship->GetAbilitySystemComponent().GetAttributes();
			attributes.SetBaseValue(OwnerAttributeIds::Luck, 0.f);
			const float baseline = f.Owner(OwnerAttributeIds::CriticalChance);
			attributes.SetBaseValue(OwnerAttributeIds::Luck, 3.f);
			f.Equal("Luck Lens observes changing total Luck", baseline + (variant == 1 ? 6.f : 0.f), f.Owner(OwnerAttributeIds::CriticalChance));
			const auto luck = attributes.AddModifier({ OwnerAttributeIds::Luck, sas::AttributeModifierOperation::Add, 2.f });
			f.Equal("Luck Lens includes Luck from other modifiers", baseline + (variant == 1 ? 10.f : 0.f), f.Owner(OwnerAttributeIds::CriticalChance));
			attributes.RemoveModifier(luck);
			f.Remove(lens);
		}
	}
}
