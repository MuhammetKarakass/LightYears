#include "gameplay/damage/DamageTypeSystem.h"

#include "gameplay/effects/GameplayEffectSystem.h"
#include "gameConfigs/combat/EffectStructs.h"
#include <algorithm>

namespace ly
{
	namespace
	{
		bool HasDamageType(const List<GameplayTag>& tags, const GameplayTag& type)
		{
			for (const GameplayTag& tag : tags)
			{
				if (tag.MatchesTag(type))
				{
					return true;
				}
			}
			return false;
		}

		void OverrideIfDeclared(
			const GameplayAttributeList& attributes,
			const GameplayTag& id,
			float& value
		)
		{
			if (const GameplayAttribute* attribute = FindGameplayAttribute(attributes, id))
			{
				value = attribute->currentValue;
			}
		}

		void OverrideIntIfDeclared(
			const GameplayAttributeList& attributes,
			const GameplayTag& id,
			int& value
		)
		{
			if (const GameplayAttribute* attribute = FindGameplayAttribute(attributes, id))
			{
				value = std::max(0, static_cast<int>(attribute->currentValue));
			}
		}

		GameplayEffectDefinition MakeIgniteEffect(const DamagePayload& payload)
		{
			GameplayEffectDefinition definition;
			definition.effectId = "Effect.Status.Damage.Ignite";
			definition.behaviorTag = DamageStatusSchema::IgniteBehavior;
			definition.durationPolicy = GameplayEffectDurationPolicy::Duration;
			definition.stackingPolicy = GameplayEffectStackingPolicy::Stack;
			definition.duration = payload.burnDuration;
			definition.maxStacks = std::max(1, payload.burnMaxStacks);
			definition.grantedTags = { DamageStatusSchema::Ignite };
			definition.attributes = {
				GameplayAttribute{ DamageAttributeIds::BurnDamagePerSecond, payload.burnDamagePerSecond, 0.f }
			};
			return definition;
		}

		GameplayEffectDefinition MakeCryoBuildupEffect(const DamagePayload& payload)
		{
			GameplayEffectDefinition definition;
			definition.effectId = DamageStatusEffectIds::CryoBuildup;
			definition.durationPolicy = GameplayEffectDurationPolicy::Duration;
			definition.stackingPolicy = GameplayEffectStackingPolicy::Stack;
			definition.duration = payload.cryoBuildupDuration;
			definition.maxStacks = std::max(1, payload.cryoBuildupRequired);
			definition.grantedTags = { DamageStatusSchema::CryoBuildup };
			return definition;
		}

		GameplayEffectDefinition MakeCryoSlowEffect(const DamagePayload& payload)
		{
			GameplayEffectDefinition definition;
			definition.effectId = DamageStatusEffectIds::CryoSlowed;
			definition.durationPolicy = GameplayEffectDurationPolicy::Duration;
			definition.stackingPolicy = GameplayEffectStackingPolicy::RefreshDuration;
			definition.duration = payload.cryoSlowDuration;
			definition.maxStacks = 1;
			definition.grantedTags = { DamageStatusSchema::CryoSlowed };
			definition.modifiers = {
				AttributeModifier{ OwnerAttributeIds::MoveSpeedHorizontal, AttributeModifierOperation::Add, -payload.cryoSlowPercent },
				AttributeModifier{ OwnerAttributeIds::MoveSpeedVertical, AttributeModifierOperation::Add, -payload.cryoSlowPercent }
			};
			return definition;
		}

		bool TryApplyCryoSlow(
			GameplayEffectSystem& targetEffects,
			const DamageContext& context
		)
		{
			if (targetEffects.FindEffectById(DamageStatusEffectIds::CryoSlowed))
			{
				// Once the four-hit threshold has been met, continued Cryo hits
				// sustain the existing slow without increasing its magnitude.
				return targetEffects.ApplyEffect(
					MakeCryoSlowEffect(context.payload),
					context.source
				).IsValid();
			}

			for (int stack = 0; stack < context.payload.cryoBuildupPerHit; ++stack)
			{
				const GameplayEffectHandle buildupHandle = targetEffects.ApplyEffect(
					MakeCryoBuildupEffect(context.payload),
					context.source
				);
				const ActiveGameplayEffect* buildup =
					targetEffects.FindEffect(buildupHandle);
				if (!buildup ||
					buildup->stackCount < context.payload.cryoBuildupRequired)
				{
					continue;
				}

				targetEffects.RemoveEffect(buildupHandle);
				return targetEffects.ApplyEffect(
					MakeCryoSlowEffect(context.payload),
					context.source
				).IsValid();
			}
			return false;
		}

		GameplayEffectDefinition MakeElectricEffect(const DamagePayload& payload)
		{
			GameplayEffectDefinition definition;
			definition.effectId = "Effect.Status.Damage.Electric";
			definition.behaviorTag = DamageStatusSchema::ElectricBehavior;
			definition.durationPolicy = GameplayEffectDurationPolicy::Duration;
			definition.stackingPolicy = GameplayEffectStackingPolicy::Stack;
			definition.duration = payload.electricDuration;
			definition.maxStacks = std::max(1, payload.electricMaxStacks);
			definition.grantedTags = { DamageStatusSchema::Electric };
			definition.attributes = {
				GameplayAttribute{
					DamageAttributeIds::ElectricDamageTakenMultiplierPerStack,
					payload.electricDamageTakenMultiplierPerStack,
					0.f
				}
			};
			return definition;
		}
	}

	DamagePayload DamageTypeSystem::BuildPayload(
		const List<GameplayTag>& damageTags,
		const GameplayAttributeList& sourceAttributes
	)
	{
		DamagePayload payload;
		if (HasDamageType(damageTags, DamageTypeSchema::Energy))
		{
			payload.shieldDamageMultiplier = 1.25f;
			payload.shieldRegenerationDelay = 0.75f;
		}
		else if (HasDamageType(damageTags, DamageTypeSchema::Kinetic))
		{
			payload.armorPenetration = 0.10f;
		}
		else if (HasDamageType(damageTags, DamageTypeSchema::Thermal))
		{
			payload.igniteStacks = 1;
			payload.burnDamagePerSecond = 1.f;
			payload.burnDuration = 3.f;
			payload.burnMaxStacks = 4;
		}
		else if (HasDamageType(damageTags, DamageTypeSchema::Cryo))
		{
			payload.cryoBuildupPerHit = 1;
			payload.cryoBuildupRequired = 4;
			payload.cryoBuildupDuration = 2.5f;
			payload.cryoSlowPercent = 0.25f;
			payload.cryoSlowDuration = 1.5f;
		}
		else if (HasDamageType(damageTags, DamageTypeSchema::Electric))
		{
			payload.electricStacks = 1;
			payload.electricDamageTakenMultiplierPerStack = 0.04f;
			payload.electricDuration = 3.f;
			payload.electricMaxStacks = 4;
		}

		OverrideIfDeclared(sourceAttributes, DamageAttributeIds::ShieldDamageMultiplier, payload.shieldDamageMultiplier);
		OverrideIfDeclared(sourceAttributes, DamageAttributeIds::ShieldRegenerationDelay, payload.shieldRegenerationDelay);
		OverrideIfDeclared(sourceAttributes, DamageAttributeIds::ArmorPenetration, payload.armorPenetration);
		OverrideIntIfDeclared(sourceAttributes, DamageAttributeIds::IgniteStacks, payload.igniteStacks);
		OverrideIfDeclared(sourceAttributes, DamageAttributeIds::BurnDamagePerSecond, payload.burnDamagePerSecond);
		OverrideIfDeclared(sourceAttributes, DamageAttributeIds::BurnDuration, payload.burnDuration);
		OverrideIntIfDeclared(sourceAttributes, DamageAttributeIds::BurnMaxStacks, payload.burnMaxStacks);
		OverrideIntIfDeclared(sourceAttributes, DamageAttributeIds::CryoBuildupPerHit, payload.cryoBuildupPerHit);
		OverrideIntIfDeclared(sourceAttributes, DamageAttributeIds::CryoBuildupRequired, payload.cryoBuildupRequired);
		OverrideIfDeclared(sourceAttributes, DamageAttributeIds::CryoBuildupDuration, payload.cryoBuildupDuration);
		OverrideIfDeclared(sourceAttributes, DamageAttributeIds::CryoSlowPercent, payload.cryoSlowPercent);
		OverrideIfDeclared(sourceAttributes, DamageAttributeIds::CryoSlowDuration, payload.cryoSlowDuration);
		OverrideIntIfDeclared(sourceAttributes, DamageAttributeIds::ElectricStacks, payload.electricStacks);
		OverrideIfDeclared(
			sourceAttributes,
			DamageAttributeIds::ElectricDamageTakenMultiplierPerStack,
			payload.electricDamageTakenMultiplierPerStack
		);
		OverrideIfDeclared(sourceAttributes, DamageAttributeIds::ElectricDuration, payload.electricDuration);
		OverrideIntIfDeclared(sourceAttributes, DamageAttributeIds::ElectricMaxStacks, payload.electricMaxStacks);

		payload.shieldDamageMultiplier = std::max(0.f, payload.shieldDamageMultiplier);
		payload.shieldRegenerationDelay = std::max(0.f, payload.shieldRegenerationDelay);
		// The three elemental effects are intentionally four-hit payoffs.  Their
		// intermediate stacks remain visible, but do not alter combat until full.
		payload.armorPenetration = std::clamp(payload.armorPenetration, 0.f, 0.25f);
		payload.igniteStacks = std::clamp(payload.igniteStacks, 0, 1);
		payload.burnDamagePerSecond = std::clamp(payload.burnDamagePerSecond, 0.f, 1.f);
		payload.burnDuration = std::max(0.f, payload.burnDuration);
		payload.burnMaxStacks = 4;
		payload.cryoBuildupPerHit = std::clamp(payload.cryoBuildupPerHit, 0, 1);
		payload.cryoBuildupRequired = 4;
		payload.cryoBuildupDuration = std::max(0.f, payload.cryoBuildupDuration);
		payload.cryoSlowPercent = std::clamp(payload.cryoSlowPercent, 0.f, 0.30f);
		payload.cryoSlowDuration = std::clamp(payload.cryoSlowDuration, 0.f, 1.5f);
		payload.electricStacks = std::clamp(payload.electricStacks, 0, 1);
		payload.electricDamageTakenMultiplierPerStack = std::clamp(
			payload.electricDamageTakenMultiplierPerStack,
			0.f,
			0.05f
		);
		payload.electricDuration = std::max(0.f, payload.electricDuration);
		payload.electricMaxStacks = 4;
		return payload;
	}

	List<GameplayTag> DamageTypeSystem::ApplyStatusEffects(
		GameplayEffectSystem& targetEffects,
		const DamageContext& context
	)
	{
		List<GameplayTag> applied;
		if (context.remainingDamage <= 0.f)
		{
			return applied;
		}

		if (context.payload.igniteStacks > 0 && context.payload.burnDamagePerSecond > 0.f && context.payload.burnDuration > 0.f)
		{
			bool igniteApplied = false;
			for (int stack = 0; stack < context.payload.igniteStacks; ++stack)
			{
				igniteApplied = targetEffects.ApplyEffect(
					MakeIgniteEffect(context.payload),
					context.source
				).IsValid() || igniteApplied;
			}
			if (igniteApplied)
			{
				applied.push_back(DamageStatusSchema::Ignite);
			}
		}
		if (context.payload.cryoBuildupPerHit > 0 &&
			context.payload.cryoBuildupRequired > 0 &&
			context.payload.cryoBuildupDuration > 0.f &&
			context.payload.cryoSlowPercent > 0.f &&
			context.payload.cryoSlowDuration > 0.f)
		{
			if (TryApplyCryoSlow(targetEffects, context))
			{
				applied.push_back(DamageStatusSchema::CryoSlowed);
			}
		}
		if (context.payload.electricStacks > 0 &&
			context.payload.electricDamageTakenMultiplierPerStack > 0.f &&
			context.payload.electricDuration > 0.f)
		{
			bool electricApplied = false;
			for (int stack = 0; stack < context.payload.electricStacks; ++stack)
			{
				electricApplied = targetEffects.ApplyEffect(
					MakeElectricEffect(context.payload),
					context.source
				).IsValid() || electricApplied;
			}
			if (electricApplied)
			{
				applied.push_back(DamageStatusSchema::Electric);
			}
		}
		return applied;
	}
}
