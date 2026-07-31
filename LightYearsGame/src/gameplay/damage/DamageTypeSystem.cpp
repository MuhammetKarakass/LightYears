#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/damage/DamageTypeSystem.h"

#include "gameConfigs/combat/EffectConfig.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
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
			const sas::GameplayAttributeList& attributes,
			const GameplayTag& id,
			float& value
		)
		{
			if (const sas::GameplayAttribute* attribute = sas::FindGameplayAttribute(attributes, id))
			{
				value = attribute->currentValue;
			}
		}

		void OverrideIntIfDeclared(
			const sas::GameplayAttributeList& attributes,
			const GameplayTag& id,
			int& value
		)
		{
			if (const sas::GameplayAttribute* attribute = sas::FindGameplayAttribute(attributes, id))
			{
				value = std::max(0, static_cast<int>(attribute->currentValue));
			}
		}

		sas::GameplayEffectSpec MakeStatusEffectSpec(
			const sas::GameplayEffectDefinition& definition,
			float duration,
			int maxStacks
		)
		{
			sas::GameplayEffectSpec spec = sas::MakeGameplayEffectSpec(definition);
			spec.duration = std::max(0.f, duration);
			spec.maxStacks = std::max(1, maxStacks);
			return spec;
		}

		bool TryApplyCryoSlow(
			sas::AbilitySystemComponent& targetAbilitySystem,
			const DamageContext& context
		)
		{
			if (targetAbilitySystem.FindGameplayEffectById(
				DamageStatusEffectIds::CryoSlowed
			))
			{
				// Once the four-hit threshold has been met, continued Cryo hits
				// sustain the existing slow without increasing its magnitude.
				sas::GameplayEffectSpec slowSpec = MakeStatusEffectSpec(
					EffectData::CryoSlowEffect,
					context.payload.cryoSlowDuration,
					1
				);
				SetGameplayEffectModifierMagnitude(
					slowSpec,
					OwnerAttributeIds::MovementSlow,
					context.payload.cryoSlowPercent
				);
				return targetAbilitySystem.ApplyGameplayEffect(
					slowSpec,
					context.source
				).IsValid();
			}

			for (int stack = 0; stack < context.payload.cryoBuildupPerHit; ++stack)
			{
				const sas::GameplayEffectSpec buildupSpec = MakeStatusEffectSpec(
					EffectData::CryoBuildupEffect,
					context.payload.cryoBuildupDuration,
					context.payload.cryoBuildupRequired
				);
				const sas::GameplayEffectHandle buildupHandle =
					targetAbilitySystem.ApplyGameplayEffect(
					buildupSpec,
					context.source
				);
				const sas::ActiveGameplayEffect* buildup =
					targetAbilitySystem.FindGameplayEffect(buildupHandle);
				if (!buildup ||
					buildup->stackCount < context.payload.cryoBuildupRequired)
				{
					continue;
				}

				targetAbilitySystem.RemoveGameplayEffect(buildupHandle);
				sas::GameplayEffectSpec slowSpec = MakeStatusEffectSpec(
					EffectData::CryoSlowEffect,
					context.payload.cryoSlowDuration,
					1
				);
				SetGameplayEffectModifierMagnitude(
					slowSpec,
					OwnerAttributeIds::MovementSlow,
					context.payload.cryoSlowPercent
				);
				return targetAbilitySystem.ApplyGameplayEffect(
					slowSpec,
					context.source
				).IsValid();
			}
			return false;
		}

		sas::GameplayEffectBehaviorResult TickIgnite(
			sas::ActiveGameplayEffect& effect,
			Actor& owner,
			float deltaTime
		)
		{
			if (deltaTime <= 0.f ||
				effect.stackCount < std::max(1, effect.spec.maxStacks))
			{
				return {};
			}
			const float damagePerSecond = std::max(
				0.f,
				sas::FindGameplayAttributeValue(
					effect.runtimeAttributes,
					DamageAttributeIds::BurnDamagePerSecond,
					0.f
				)
			);
			if (damagePerSecond > 0.f)
			{
				ApplyCombatDamage(
					owner,
					damagePerSecond * static_cast<float>(effect.stackCount) * deltaTime,
					effect.GetSourceObject<Actor>(),
					{ DamageTypeSchema::Thermal }
				);
			}
			return {};
		}

		sas::GameplayEffectBehaviorResult ProcessElectricIncomingDamage(
			sas::ActiveGameplayEffect& effect,
			DamageContext& context
		)
		{
			if (effect.stackCount < std::max(1, effect.spec.maxStacks))
			{
				return {};
			}
			const float multiplierPerStack = std::max(
				0.f,
				sas::FindGameplayAttributeValue(
					effect.runtimeAttributes,
					DamageAttributeIds::ElectricDamageTakenMultiplierPerStack,
					0.f
				)
			);
			context.remainingDamage *=
				1.f + multiplierPerStack * static_cast<float>(effect.stackCount);
			return {};
		}
	}

	DamagePayload DamageTypeSystem::BuildPayload(
		const List<GameplayTag>& damageTags,
		const sas::GameplayAttributeList& sourceAttributes
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
		sas::AbilitySystemComponent& targetAbilitySystem,
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
			sas::GameplayEffectSpec igniteSpec = MakeStatusEffectSpec(
				EffectData::IgniteEffect,
				context.payload.burnDuration,
				context.payload.burnMaxStacks
			);
			SetGameplayEffectAttributeBaseValue(
				igniteSpec,
				DamageAttributeIds::BurnDamagePerSecond,
				context.payload.burnDamagePerSecond
			);
			bool igniteApplied = false;
			for (int stack = 0; stack < context.payload.igniteStacks; ++stack)
			{
				igniteApplied = targetAbilitySystem.ApplyGameplayEffect(
					igniteSpec,
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
			if (TryApplyCryoSlow(targetAbilitySystem, context))
			{
				applied.push_back(DamageStatusSchema::CryoSlowed);
			}
		}
		if (context.payload.electricStacks > 0 &&
			context.payload.electricDamageTakenMultiplierPerStack > 0.f &&
			context.payload.electricDuration > 0.f)
		{
			sas::GameplayEffectSpec electricSpec = MakeStatusEffectSpec(
				EffectData::ElectricEffect,
				context.payload.electricDuration,
				context.payload.electricMaxStacks
			);
			SetGameplayEffectAttributeBaseValue(
				electricSpec,
				DamageAttributeIds::ElectricDamageTakenMultiplierPerStack,
				context.payload.electricDamageTakenMultiplierPerStack
			);
			bool electricApplied = false;
			for (int stack = 0; stack < context.payload.electricStacks; ++stack)
			{
				electricApplied = targetAbilitySystem.ApplyGameplayEffect(
					electricSpec,
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

	bool DamageTypeSystem::RegisterDamageEffectBehaviors()
	{
		static const bool registered = []
		{
			LightYearsAbilitySystemComponent::
				EffectBehaviorRuntime::Hooks igniteHooks;
			igniteHooks.tick = &TickIgnite;
			const bool igniteRegistered =
				LightYearsAbilitySystemComponent::
					GetEffectBehaviorRuntime().Register(
						DamageStatusSchema::IgniteBehavior,
						igniteHooks
					);

			LightYearsAbilitySystemComponent::
				EffectBehaviorRuntime::Hooks electricHooks;
			electricHooks.eventPhase =
				LightYearsAbilitySystemComponent::
					IncomingDamagePhase::PreMitigation;
			electricHooks.processEvent = &ProcessElectricIncomingDamage;
			const bool electricRegistered =
				LightYearsAbilitySystemComponent::
					GetEffectBehaviorRuntime().Register(
						DamageStatusSchema::ElectricBehavior,
						electricHooks
					);
			return igniteRegistered && electricRegistered;
		}();
		return registered;
	}
}
