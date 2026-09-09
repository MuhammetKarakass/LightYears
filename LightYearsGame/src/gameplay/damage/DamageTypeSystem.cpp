#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/damage/DamageTypeSystem.h"

#include "gameConfigs/combat/EffectConfig.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/effects/LightYearsEffectBehaviorRuntime.h"
#include "gameplay/time/PeriodicTickAccumulator.h"
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
			const sas::AttributeId& id,
			float& value
		)
		{
			if (const sas::GameplayAttribute* attribute = sas::FindAttribute(attributes, id))
			{
				value = attribute->currentValue;
			}
		}

		void OverrideIntIfDeclared(
			const sas::GameplayAttributeList& attributes,
			const sas::AttributeId& id,
			int& value
		)
		{
			if (const sas::GameplayAttribute* attribute = sas::FindAttribute(attributes, id))
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

		sas::GameplayEffectSpec MakeMovementSlowSpec(
			const sas::GameplayEffectDefinition& definition,
			float duration,
			float magnitude
		)
		{
			sas::GameplayEffectSpec spec = MakeStatusEffectSpec(definition, duration, 1);
			spec.modifiers = {
				sas::AttributeModifier{
					OwnerAttributeIds::MovementSlow,
					sas::AttributeModifierOperation::Add,
					magnitude
				}
			};
			return spec;
		}

		struct CappedStackApplicationResult
		{
			bool wasApplied = false;
			int stackCount = 0;
			int maxStacks = 1;

			bool IsAtThreshold() const
			{
				return stackCount >= maxStacks;
			}
		};

		// All stack-based damage statuses share this lifecycle: applications add up
		// to their cap, then remain at that cap while later applications refresh
		// duration. A threshold effect is therefore sustained, never consumed.
		CappedStackApplicationResult ApplyCappedStackEffect(
			sas::AbilitySystemComponent& targetAbilitySystem,
			const sas::GameplayEffectSpec& spec,
			Actor* source,
			int incomingStacks
		)
		{
			CappedStackApplicationResult result;
			result.maxStacks = std::max(1, spec.maxStacks);
			for (int stack = 0; stack < std::max(0, incomingStacks); ++stack)
			{
				result.wasApplied = targetAbilitySystem.ApplyGameplayEffect(
					spec,
					source
				).IsValid() || result.wasApplied;
			}

			if (const sas::ActiveGameplayEffect* active =
				targetAbilitySystem.FindGameplayEffectById(spec.definition.effectId))
			{
				result.stackCount = active->stackCount;
				result.maxStacks = std::max(1, active->spec.maxStacks);
			}
			return result;
		}

		float GetMovementSlowMagnitude(const sas::ActiveGameplayEffect& effect)
		{
			for (const sas::AttributeModifier& modifier : effect.spec.modifiers)
			{
				if (modifier.attributeId == OwnerAttributeIds::MovementSlow &&
					modifier.operation == sas::AttributeModifierOperation::Add)
				{
					return std::clamp(modifier.magnitude, 0.f, 1.f);
				}
			}
			return 0.f;
		}

		bool TryApplyOrRefreshCryoSlow(
			sas::AbilitySystemComponent& targetAbilitySystem,
			const sas::GameplayEffectDefinition& slowDefinition,
			const DamageContext& context
		)
		{
			if (const sas::ActiveGameplayEffect* activeSlow =
				targetAbilitySystem.FindGameplayEffectById(
					DamageStatusEffectIds::CryoSlowedEffectId
				))
			{
				constexpr float MagnitudeEqualityTolerance = 0.0001f;
				const float activeMagnitude = GetMovementSlowMagnitude(*activeSlow);
				if (context.payload.cryoSlowPercent + MagnitudeEqualityTolerance <
					activeMagnitude)
				{
					// A weaker Cryo hit keeps the stronger effect and cannot extend it.
					return false;
				}
			}

			// A stronger slow replaces the old one; an equal slow refreshes its own
			// duration through the shared RefreshDuration stacking policy.
			return targetAbilitySystem.ApplyGameplayEffect(
				MakeMovementSlowSpec(
					slowDefinition,
					context.payload.cryoSlowDuration,
					context.payload.cryoSlowPercent
				),
				context.source
			).IsValid();
		}

		bool TryApplyCryoSlow(
			sas::AbilitySystemComponent& targetAbilitySystem,
			const DamageContext& context
		)
		{
			const sas::GameplayEffectDefinition* slowDefinition =
				EffectData::FindGameplayEffectDefinition(DamageStatusEffectIds::CryoSlowedEffectId);
			const sas::GameplayEffectDefinition* buildupDefinition =
				EffectData::FindGameplayEffectDefinition(DamageStatusEffectIds::CryoBuildupEffectId);
			if (!slowDefinition || !buildupDefinition)
			{
				return false;
			}

			const CappedStackApplicationResult buildup = ApplyCappedStackEffect(
				targetAbilitySystem,
				MakeStatusEffectSpec(
					*buildupDefinition,
					context.payload.cryoBuildupDuration,
					context.payload.cryoBuildupRequired
				),
				context.source,
				context.payload.cryoBuildupPerHit
			);
			return buildup.wasApplied && buildup.IsAtThreshold() &&
				TryApplyOrRefreshCryoSlow(
					targetAbilitySystem,
					*slowDefinition,
					context
				);
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

			const float damagePerTick = std::max(
				0.f,
				sas::FindAttributeValue(
					effect.runtimeAttributes,
					DamageAttributeIds::BurnDamagePerTick,
					0.f
				)
			);
			const float tickInterval = std::max(
				0.f,
				sas::FindAttributeValue(
					effect.runtimeAttributes,
					DamageAttributeIds::BurnTickInterval,
					0.f
				)
			);
			if (damagePerTick > 0.f && tickInterval > 0.f)
			{
				// Periodic Burn is an optional generic mode. It is deliberately
				// independent of Ignite stack count: four stacks unlock the status,
				// but do not multiply the snapshotted Scorch Drive damage.
				sas::GameplayAttribute* accumulator = sas::FindAttribute(
					effect.runtimeAttributes,
					DamageAttributeIds::BurnTickAccumulator
				);
				if (!accumulator)
				{
					effect.runtimeAttributes.emplace_back(
						DamageAttributeIds::BurnTickAccumulator,
						0.f,
						0.f
					);
					accumulator = sas::FindAttribute(
						effect.runtimeAttributes,
						DamageAttributeIds::BurnTickAccumulator
					);
				}
				if (!accumulator)
				{
					return {};
				}

				const int tickCount = time::ConsumePeriodicTicks(
					accumulator->currentValue,
					deltaTime,
					tickInterval
				);
				for (int tickIndex = 0; tickIndex < tickCount; ++tickIndex)
				{
					// Preserve player ownership for periodic effect damage while its
					// source actor is still available. If the source has already gone
					// away, the hit remains valid damage but is intentionally unowned.
					Actor* effectSource = effect.GetSourceObject<Actor>();
					ApplyCombatDamage(
						owner,
						damagePerTick,
						effectSource,
						{ DamageTypeSchema::Thermal }
					);
				}
				return {};
			}

			const float damagePerSecond = std::max(
				0.f,
				sas::FindAttributeValue(
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
				sas::FindAttributeValue(
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
		// Damage tags provide identity only. Every balance value below is supplied
		// by the owning weapon, ability or enemy through sourceAttributes.

		if (HasDamageType(damageTags, DamageTypeSchema::Energy))
		{
			OverrideIfDeclared(sourceAttributes, DamageAttributeIds::ShieldDamageMultiplier, payload.shieldDamageMultiplier);
			OverrideIfDeclared(sourceAttributes, DamageAttributeIds::ShieldRegenerationDelay, payload.shieldRegenerationDelay);
		}
		if (HasDamageType(damageTags, DamageTypeSchema::Kinetic))
		{
			OverrideIfDeclared(sourceAttributes, DamageAttributeIds::ArmorPenetration, payload.armorPenetration);
		}
		if (HasDamageType(damageTags, DamageTypeSchema::Thermal))
		{
			OverrideIntIfDeclared(sourceAttributes, DamageAttributeIds::IgniteStacks, payload.igniteStacks);
			OverrideIfDeclared(sourceAttributes, DamageAttributeIds::BurnDamagePerSecond, payload.burnDamagePerSecond);
			OverrideIfDeclared(sourceAttributes, DamageAttributeIds::BurnDamagePerTick, payload.burnDamagePerTick);
			OverrideIfDeclared(sourceAttributes, DamageAttributeIds::BurnTickInterval, payload.burnTickInterval);
			OverrideIfDeclared(sourceAttributes, DamageAttributeIds::BurnDuration, payload.burnDuration);
			OverrideIntIfDeclared(sourceAttributes, DamageAttributeIds::BurnMaxStacks, payload.burnMaxStacks);
		}
		if (HasDamageType(damageTags, DamageTypeSchema::Cryo))
		{
			OverrideIntIfDeclared(sourceAttributes, DamageAttributeIds::CryoBuildupPerHit, payload.cryoBuildupPerHit);
			OverrideIntIfDeclared(sourceAttributes, DamageAttributeIds::CryoBuildupRequired, payload.cryoBuildupRequired);
			OverrideIfDeclared(sourceAttributes, DamageAttributeIds::CryoBuildupDuration, payload.cryoBuildupDuration);
			OverrideIfDeclared(sourceAttributes, DamageAttributeIds::CryoSlowPercent, payload.cryoSlowPercent);
			OverrideIfDeclared(sourceAttributes, DamageAttributeIds::CryoSlowDuration, payload.cryoSlowDuration);
		}
		if (HasDamageType(damageTags, DamageTypeSchema::Electric))
		{
			OverrideIntIfDeclared(sourceAttributes, DamageAttributeIds::ElectricStacks, payload.electricStacks);
			OverrideIfDeclared(sourceAttributes, DamageAttributeIds::ElectricDamageTakenMultiplierPerStack, payload.electricDamageTakenMultiplierPerStack);
			OverrideIfDeclared(sourceAttributes, DamageAttributeIds::ElectricDuration, payload.electricDuration);
			OverrideIntIfDeclared(sourceAttributes, DamageAttributeIds::ElectricMaxStacks, payload.electricMaxStacks);
		}

		payload.shieldDamageMultiplier = std::max(0.f, payload.shieldDamageMultiplier);
		payload.shieldRegenerationDelay = std::max(0.f, payload.shieldRegenerationDelay);
		// Generic payload sanitation must not encode one shipped balance profile.
		// Family/content validation owns those caps; the runtime only enforces
		// mathematically safe ranges and relationships between payload fields.
		payload.armorPenetration = std::clamp(payload.armorPenetration, 0.f, 1.f);
		payload.burnMaxStacks = std::max(1, payload.burnMaxStacks);
		payload.igniteStacks = std::clamp(
			payload.igniteStacks,
			0,
			payload.burnMaxStacks
		);
		payload.burnDamagePerSecond = std::max(0.f, payload.burnDamagePerSecond);
		payload.burnDamagePerTick = std::max(0.f, payload.burnDamagePerTick);
		payload.burnTickInterval = std::max(0.f, payload.burnTickInterval);
		payload.burnDuration = std::max(0.f, payload.burnDuration);
		payload.cryoBuildupRequired = std::max(1, payload.cryoBuildupRequired);
		payload.cryoBuildupPerHit = std::clamp(
			payload.cryoBuildupPerHit,
			0,
			payload.cryoBuildupRequired
		);
		payload.cryoBuildupDuration = std::max(0.f, payload.cryoBuildupDuration);
		payload.cryoSlowPercent = std::clamp(payload.cryoSlowPercent, 0.f, 1.f);
		payload.cryoSlowDuration = std::max(0.f, payload.cryoSlowDuration);
		payload.electricDamageTakenMultiplierPerStack = std::clamp(
			payload.electricDamageTakenMultiplierPerStack,
			0.f,
			1.f
		);
		payload.electricDuration = std::max(0.f, payload.electricDuration);
		payload.electricMaxStacks = std::max(1, payload.electricMaxStacks);
		payload.electricStacks = std::clamp(
			payload.electricStacks,
			0,
			payload.electricMaxStacks
		);
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

		const bool hasPeriodicBurn =
			context.payload.burnDamagePerTick > 0.f &&
			context.payload.burnTickInterval > 0.f;
		if (context.payload.igniteStacks > 0 &&
			(context.payload.burnDamagePerSecond > 0.f || hasPeriodicBurn) &&
			context.payload.burnDuration > 0.f)
		{
			const sas::GameplayEffectDefinition* igniteDefinition =
				EffectData::FindGameplayEffectDefinition("Effect.Status.Damage.Ignite");
			if (igniteDefinition)
			{
				sas::GameplayEffectSpec igniteSpec = MakeStatusEffectSpec(
					*igniteDefinition,
					context.payload.burnDuration,
					context.payload.burnMaxStacks
				);
				igniteSpec.attributes.clear();
				if (context.payload.burnDamagePerSecond > 0.f)
				{
					igniteSpec.attributes.emplace_back(
						DamageAttributeIds::BurnDamagePerSecond,
						context.payload.burnDamagePerSecond,
						0.f
					);
				}
				if (hasPeriodicBurn)
				{
					igniteSpec.attributes.emplace_back(
						DamageAttributeIds::BurnDamagePerTick,
						context.payload.burnDamagePerTick,
						0.f
					);
					igniteSpec.attributes.emplace_back(
						DamageAttributeIds::BurnTickInterval,
						context.payload.burnTickInterval,
						0.001f
					);
					igniteSpec.attributes.emplace_back(
						DamageAttributeIds::BurnTickAccumulator,
						0.f,
						0.f
					);
				}
				if (ApplyCappedStackEffect(
					targetAbilitySystem,
					igniteSpec,
					context.source,
					context.payload.igniteStacks
				).wasApplied)
				{
					applied.push_back(DamageStatusSchema::Ignite);
				}
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
			const sas::GameplayEffectDefinition* electricDefinition =
				EffectData::FindGameplayEffectDefinition("Effect.Status.Damage.Electric");
			if (electricDefinition)
			{
				sas::GameplayEffectSpec electricSpec = MakeStatusEffectSpec(
					*electricDefinition,
					context.payload.electricDuration,
					context.payload.electricMaxStacks
				);
				electricSpec.attributes = {
					sas::GameplayAttribute{
						DamageAttributeIds::ElectricDamageTakenMultiplierPerStack,
						context.payload.electricDamageTakenMultiplierPerStack,
						0.f
					}
				};
				if (ApplyCappedStackEffect(
					targetAbilitySystem,
					electricSpec,
					context.source,
					context.payload.electricStacks
				).wasApplied)
				{
					applied.push_back(DamageStatusSchema::Electric);
				}
			}
		}
		return applied;
	}

	bool DamageTypeSystem::RegisterDamageEffectBehaviors()
	{
		static const bool registered = []
		{
			LightYearsEffectBehaviorRuntime::Hooks igniteHooks;
			igniteHooks.tick = &TickIgnite;
			const bool igniteRegistered =
				GetEffectBehaviorRuntime().Register(
						EffectData::DamageIgniteBehaviorKey,
						igniteHooks
					);

			LightYearsEffectBehaviorRuntime::Hooks electricHooks;
			electricHooks.eventPhase =
				IncomingDamagePhase::PreMitigation;
			electricHooks.processEvent = &ProcessElectricIncomingDamage;
			const bool electricRegistered =
				GetEffectBehaviorRuntime().Register(
						EffectData::DamageElectricBehaviorKey,
						electricHooks
					);
			return igniteRegistered && electricRegistered;
		}();
		return registered;
	}
}
