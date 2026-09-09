#pragma once

#include "gameplay/ability/blastback/BlastbackContracts.h"
#include "gameplay/ability/content/GameAbilityProgression.h"

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition Blastback_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::Blastback::AbilityId::Basic;
		// Runtime loadouts own the actual binding. Ability1 is only the catalog
		// default required by the current content contract.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.cooldown = 9.f;
		// Duration is precisely the input-locked focus time. Recoil continues in
		// a separate actor so it cannot delay cooldown start.
		definition.duration = 0.5f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::Blastback
		};
		definition.displayName = "Blastback";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupRed.png";
		definition.accentColor = sf::Color{ 255, 105, 48, 255 };
		definition.attributes = {
			sas::GameplayAttribute{ AbilityData::Blastback::Attribute::Damage, 25.f, 0.f },
			sas::GameplayAttribute{ AbilityData::Blastback::Attribute::Range, 400.f, 1.f },
			sas::GameplayAttribute{ AbilityData::Blastback::Attribute::InnerRange, 200.f, 0.f },
			sas::GameplayAttribute{ AbilityData::Blastback::Attribute::ConeHalfAngleDegrees, 32.f, 1.f, 89.f },
			sas::GameplayAttribute{ AbilityData::Blastback::Attribute::InnerDamageMultiplier, 2.f, 1.f },
			sas::GameplayAttribute{ AbilityData::Blastback::Attribute::InnerIgniteStacks, 4.f, 1.f, 4.f },
			sas::GameplayAttribute{ AbilityData::Blastback::Attribute::OuterIgniteStacks, 2.f, 1.f, 4.f },
			sas::GameplayAttribute{ AbilityData::Blastback::Attribute::InnerStunDuration, 1.f, 0.f },
			sas::GameplayAttribute{ AbilityData::Blastback::Attribute::OuterStunDuration, 0.5f, 0.f },
			sas::GameplayAttribute{ AbilityData::Blastback::Attribute::MaxHealthReference, 100.f, 0.f },
			sas::GameplayAttribute{ AbilityData::Blastback::Attribute::MaxHealthStunScale, 0.0005f, 0.f },
			sas::GameplayAttribute{ AbilityData::Blastback::Attribute::MinimumPushInitialSpeed, 240.f, 0.f },
			sas::GameplayAttribute{ AbilityData::Blastback::Attribute::MaximumPushInitialSpeed, 760.f, 0.f },
			sas::GameplayAttribute{ AbilityData::Blastback::Attribute::InnerPushMultiplier, 1.2f, 1.f },
			sas::GameplayAttribute{ AbilityData::Blastback::Attribute::RecoilInitialSpeed, 800.f, 0.f },
			sas::GameplayAttribute{ AbilityData::Blastback::Attribute::RecoilDuration, 0.5f, 0.01f },
			// Inner/outer stack count is selected at hit time; these values define
			// the shared Thermal status instance that receives those stacks.
			sas::GameplayAttribute{ AbilityData::Blastback::Attribute::BurnDamagePerSecond, 1.f, 0.f },
			sas::GameplayAttribute{ AbilityData::Blastback::Attribute::BurnDuration, 3.f, 0.f },
			sas::GameplayAttribute{ AbilityData::Blastback::Attribute::BurnMaxStacks, 4.f, 1.f }
		};
		definition.scalingRules = {
			sas::AttributeScalingRule{
				AbilityData::Blastback::Attribute::Damage,
				ly::OwnerAttributeIds::AttackPower,
				sas::AttributeModifierOperation::Add,
				0.75f
			}
		};
		definition.levelProgression = ly::MakeRepeatedAbilityLevelProgression(
			14,
			ly::AbilityLevelStep{
				{
					sas::AttributeModifier{
						AbilityData::Blastback::Attribute::Damage,
						sas::AttributeModifierOperation::Add,
						3.f
					},
					sas::AttributeModifier{
						ly::CommonAttributeIds::Cooldown,
						sas::AttributeModifierOperation::Add,
						-0.2f
					}
				}, {}, {}, {}
			}
		);
		definition.levelUpgradeScrapCosts = {
			60, 60, 60, 60, 60, 60, 60,
			60, 60, 60, 60, 60, 60, 60
		};
		definition.damageTags = { ly::DamageTypeSchema::Thermal };
		definition.behaviorType = ly::AbilityBehaviorType::Blastback;
		return definition;
	}();
}
