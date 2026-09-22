#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/damage/DamageTypeSystem.h"

#include "gameConfigs/combat/EffectConfig.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/content/DamageStatusBalanceCatalog.h"
#include "gameplay/effects/LightYearsEffectBehaviorRuntime.h"
#include "gameplay/time/PeriodicTickAccumulator.h"
#include "effects/GameplayEffectBindings.h"
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

		const DamageStatusBalance& GetDamageStatusBalance()
		{
			return content::DamageStatusBalanceCatalog::Get();
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
			const sas::GameplayEffectDefinition& definition, float duration, int maxStacks
		)
		{
			sas::GameplayEffectSpec spec = sas::MakeGameplayEffectSpec(definition);
			spec.duration = std::max(0.f, duration);
			spec.maxStacks = std::max(1, maxStacks);
			return spec;
		}

		sas::GameplayEffectSpec MakeMovementSlowSpec(
			const sas::GameplayEffectDefinition& definition, float duration, int maxStacks, float magnitude
		)
		{
			sas::GameplayEffectSpec spec = MakeStatusEffectSpec(definition, duration, maxStacks);
			spec.modifiers = { sas::AttributeModifier{ OwnerAttributeIds::MovementSlow, sas::AttributeModifierOperation::Add, magnitude } };
			return spec;
		}

		// All selected damage statuses use the ASC's ordinary Stack policy. The
		// opt-in stack lifetime policy owns decay; this helper only applies hits.
		bool ApplyCappedStackEffect(
			sas::AbilitySystemComponent& targetAbilitySystem,
			const sas::GameplayEffectSpec& spec,
			Actor* source,
			int incomingStacks
		)
		{
			bool applied = false;
			for (int stack = 0; stack < std::max(0, incomingStacks); ++stack)
			{
				applied = targetAbilitySystem.ApplyGameplayEffect(spec, source).IsValid() || applied;
			}
			return applied;
		}

		bool ApplyCryoStatus(
			sas::AbilitySystemComponent& targetAbilitySystem,
			const DamageContext& context
		)
		{
			const sas::GameplayEffectDefinition* slowDefinition =
				EffectData::FindGameplayEffectDefinition(DamageStatusEffectIds::CryoSlowedEffectId);
			if (!slowDefinition)
			{
				return false;
			}
			return ApplyCappedStackEffect(
				targetAbilitySystem,
				MakeMovementSlowSpec(
					*slowDefinition,
					GetDamageStatusBalance().cryo.duration,
					GetDamageStatusBalance().cryo.maxStacks,
					GetDamageStatusBalance().CryoSlowPercent(1)
				),
				context.source,
				context.payload.cryoBuildupPerHit
			);
		}

		sas::GameplayEffectBehaviorResult TickIgnite(
			sas::ActiveGameplayEffect& effect,
			Actor& owner,
			float deltaTime
		)
		{
			if (deltaTime <= 0.f)
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
				// Periodic Burn is an explicit source-owned mode. It is independent of
				// the canonical Thermal DPS table and therefore returns after ticking.
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

			const float damagePerSecond = GetDamageStatusBalance().ThermalDamagePerSecond(
				effect.stackCount
			);
			if (damagePerSecond > 0.f)
			{
				ApplyCombatDamage(
					owner,
					damagePerSecond * deltaTime,
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
			context.remainingDamage *=
				1.f + GetDamageStatusBalance().ElectricDamageTakenMultiplier(effect.stackCount);
			return {};
		}

		bool ApplyKineticStatus(
			sas::AbilitySystemComponent& targetAbilitySystem,
			DamageContext& context
		)
		{
			const sas::ActiveGameplayEffect* active = targetAbilitySystem.FindGameplayEffectById(
				DamageStatusEffectIds::KineticEffectId
			);
			if (active && active->stackCount > 0)
			{
				context.payload.armorPenetration = std::clamp(
					context.payload.armorPenetration +
					GetDamageStatusBalance().KineticArmorPenetration(active->stackCount),
					0.f,
					1.f
				);
			}
			const sas::GameplayEffectDefinition* definition =
				EffectData::FindGameplayEffectDefinition(DamageStatusEffectIds::KineticEffectId);
			if (!definition)
			{
				return false;
			}
			return ApplyCappedStackEffect(
				targetAbilitySystem,
				MakeStatusEffectSpec(
					*definition,
					GetDamageStatusBalance().kinetic.duration,
					GetDamageStatusBalance().kinetic.maxStacks
				),
				context.source,
				context.payload.kineticStacks
			);
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
			payload.shieldDamageMultiplier = GetDamageStatusBalance().energyShieldDamageMultiplier;
			if (const sas::GameplayAttribute* attribute = sas::FindAttribute(
				sourceAttributes,
				DamageAttributeIds::ShieldDamageMultiplier
			))
			{
				payload.shieldDamageMultiplier = std::max(
					payload.shieldDamageMultiplier,
					attribute->currentValue
				);
			}
			OverrideIfDeclared(sourceAttributes, DamageAttributeIds::ShieldRegenerationDelay, payload.shieldRegenerationDelay);
		}
		if (HasDamageType(damageTags, DamageTypeSchema::Kinetic))
		{
			OverrideIfDeclared(sourceAttributes, DamageAttributeIds::ArmorPenetration, payload.armorPenetration);
			OverrideIntIfDeclared(sourceAttributes, DamageAttributeIds::KineticStacks, payload.kineticStacks);
		}
		if (HasDamageType(damageTags, DamageTypeSchema::Thermal))
		{
			OverrideIntIfDeclared(sourceAttributes, DamageAttributeIds::IgniteStacks, payload.igniteStacks);
			OverrideIfDeclared(sourceAttributes, DamageAttributeIds::BurnDamagePerTick, payload.burnDamagePerTick);
			OverrideIfDeclared(sourceAttributes, DamageAttributeIds::BurnTickInterval, payload.burnTickInterval);
			OverrideIfDeclared(sourceAttributes, DamageAttributeIds::BurnDuration, payload.burnDuration);
		}
		if (HasDamageType(damageTags, DamageTypeSchema::Cryo))
		{
			OverrideIntIfDeclared(sourceAttributes, DamageAttributeIds::CryoBuildupPerHit, payload.cryoBuildupPerHit);
		}
		if (HasDamageType(damageTags, DamageTypeSchema::Electric))
		{
			OverrideIntIfDeclared(sourceAttributes, DamageAttributeIds::ElectricStacks, payload.electricStacks);
		}

		payload.shieldDamageMultiplier = std::max(0.f, payload.shieldDamageMultiplier);
		payload.shieldRegenerationDelay = std::max(0.f, payload.shieldRegenerationDelay);
		// Generic payload sanitation must not encode one shipped balance profile.
		// Family/content validation owns those caps; the runtime only enforces
		// mathematically safe ranges and relationships between payload fields.
		payload.armorPenetration = std::clamp(payload.armorPenetration, 0.f, 1.f);
		payload.igniteStacks = std::clamp(
			payload.igniteStacks,
			0,
			GetDamageStatusBalance().thermal.maxStacks
		);
		payload.burnDamagePerTick = std::max(0.f, payload.burnDamagePerTick);
		payload.burnTickInterval = std::max(0.f, payload.burnTickInterval);
		payload.burnDuration = std::max(0.f, payload.burnDuration);
		payload.cryoBuildupPerHit = std::clamp(
			payload.cryoBuildupPerHit,
			0,
			GetDamageStatusBalance().cryo.maxStacks
		);
		payload.electricStacks = std::clamp(
			payload.electricStacks,
			0,
			GetDamageStatusBalance().electric.maxStacks
		);
		payload.kineticStacks = std::clamp(
			payload.kineticStacks,
			0,
			GetDamageStatusBalance().kinetic.maxStacks
		);
		return payload;
	}

	List<GameplayTag> DamageTypeSystem::ApplyStatusEffects(
		sas::AbilitySystemComponent& targetAbilitySystem,
		DamageContext& context
	)
	{
		List<GameplayTag> applied;
		if (context.remainingDamage <= 0.f)
		{
			return applied;
		}

		const bool hasPeriodicBurnDamage = context.payload.burnDamagePerTick > 0.f;
		const bool hasPeriodicBurnInterval = context.payload.burnTickInterval > 0.f;
		const bool hasPeriodicBurn = hasPeriodicBurnDamage && hasPeriodicBurnInterval;
		const bool hasInvalidPeriodicBurn =
			(hasPeriodicBurnDamage || hasPeriodicBurnInterval || context.payload.burnDuration > 0.f) &&
			!hasPeriodicBurn;
		if (HasDamageType(context.damageTags, DamageTypeSchema::Thermal) &&
			context.payload.igniteStacks > 0 && !hasInvalidPeriodicBurn)
		{
			const sas::GameplayEffectDefinition* igniteDefinition =
				EffectData::FindGameplayEffectDefinition("Effect.Status.Damage.Ignite");
			if (igniteDefinition)
			{
				sas::GameplayEffectSpec igniteSpec = MakeStatusEffectSpec(
					*igniteDefinition,
					hasPeriodicBurn && context.payload.burnDuration > 0.f
					? context.payload.burnDuration
					: GetDamageStatusBalance().thermal.duration,
					GetDamageStatusBalance().thermal.maxStacks
				);
				igniteSpec.attributes.clear();
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
				))
				{
					applied.push_back(DamageStatusSchema::Ignite);
				}
			}
		}
		if (HasDamageType(context.damageTags, DamageTypeSchema::Cryo) &&
			context.payload.cryoBuildupPerHit > 0)
		{
			if (ApplyCryoStatus(targetAbilitySystem, context))
			{
				applied.push_back(DamageStatusSchema::CryoSlowed);
			}
		}
		if (HasDamageType(context.damageTags, DamageTypeSchema::Electric) &&
			context.payload.electricStacks > 0)
		{
			const sas::GameplayEffectDefinition* electricDefinition =
				EffectData::FindGameplayEffectDefinition("Effect.Status.Damage.Electric");
			if (electricDefinition)
			{
				sas::GameplayEffectSpec electricSpec = MakeStatusEffectSpec(
					*electricDefinition,
					GetDamageStatusBalance().electric.duration,
					GetDamageStatusBalance().electric.maxStacks
				);
				electricSpec.attributes.clear();
				if (ApplyCappedStackEffect(
					targetAbilitySystem,
					electricSpec,
					context.source,
					context.payload.electricStacks
				))
				{
					applied.push_back(DamageStatusSchema::Electric);
				}
			}
		}
		if (HasDamageType(context.damageTags, DamageTypeSchema::Kinetic) &&
			ApplyKineticStatus(targetAbilitySystem, context))
		{
			applied.push_back(DamageStatusSchema::Kinetic);
		}
		return applied;
	}

	void DamageTypeSystem::SynchronizeStatusEffect(
		sas::AbilitySystemComponent& targetAbilitySystem,
		sas::ActiveGameplayEffect& effect
	)
	{
		if (effect.spec.definition.effectId != DamageStatusEffectIds::CryoSlowedEffectId)
		{
			return;
		}
		sas::RemoveGameplayEffectModifiers(effect, targetAbilitySystem.GetAttributes());
		if (effect.stackCount <= 0)
		{
			return;
		}
		effect.spec.modifiers = {
			sas::AttributeModifier{
				OwnerAttributeIds::MovementSlow,
				sas::AttributeModifierOperation::Add,
				GetDamageStatusBalance().CryoSlowPercent(effect.stackCount)
			}
		};
		sas::ApplyGameplayEffectModifiers(
			effect.spec,
			effect,
			targetAbilitySystem.GetAttributes()
		);
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
