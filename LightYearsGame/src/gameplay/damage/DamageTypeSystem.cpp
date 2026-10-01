#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/damage/DamageTypeSystem.h"

#include "gameConfigs/combat/CombatTick.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/content/DamageStatusBalanceCatalog.h"
#include "gameplay/effects/LightYearsEffectBehaviorRuntime.h"
#include "gameplay/time/PeriodicTickAccumulator.h"
#include "effects/GameplayEffectBindings.h"
#include <algorithm>
#include <cmath>

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

		void EnsureRuntimeAttribute(sas::ActiveGameplayEffect& effect, const sas::AttributeId& id)
		{
			if (!sas::FindAttribute(effect.runtimeAttributes, id))
			{
				effect.runtimeAttributes.emplace_back(id, 0.f, 0.f);
			}
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

			// Canonical Ignite is authored as damage per second. Integrate it every
			// slice with the stack count that was active for that slice, but deliver it
			// once per global combat tick so total damage is unchanged while the hit
			// cadence matches every other periodic source.
			const float damagePerSecond = GetDamageStatusBalance().ThermalDamagePerSecond(
				effect.stackCount
			);
			// Adding an attribute can reallocate the vector, so create both before
			// taking any pointer into it.
			EnsureRuntimeAttribute(effect, DamageAttributeIds::IgnitePendingDamage);
			EnsureRuntimeAttribute(effect, DamageAttributeIds::IgniteTickAccumulator);
			sas::GameplayAttribute* pendingDamage = sas::FindAttribute(
				effect.runtimeAttributes, DamageAttributeIds::IgnitePendingDamage
			);
			sas::GameplayAttribute* tickClock = sas::FindAttribute(
				effect.runtimeAttributes, DamageAttributeIds::IgniteTickAccumulator
			);
			if (!pendingDamage || !tickClock)
			{
				return {};
			}

			pendingDamage->currentValue += std::max(0.f, damagePerSecond) * deltaTime;
			const int dueTicks = time::ConsumePeriodicTicks(
				tickClock->currentValue,
				deltaTime,
				CombatTick::Interval
			);
			// All integrated damage is delivered in one hit, so a hitch must not leave
			// tick debt behind that would make later frames hit off-cadence.
			tickClock->currentValue = std::fmod(tickClock->currentValue, CombatTick::Interval);
			// The last slice of the last stack removes the effect and there is no
			// remove hook, so deliver the remainder now instead of dropping it.
			const bool endsAfterThisSlice =
				effect.stackCount <= 1 && effect.remainingDuration <= deltaTime + 0.0001f;
			if ((dueTicks > 0 || endsAfterThisSlice) && pendingDamage->currentValue > 0.f)
			{
				const float damage = pendingDamage->currentValue;
				pendingDamage->currentValue = 0.f;
				ApplyCombatDamage(
					owner,
					damage,
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

		if (HasDamageType(context.damageTags, DamageTypeSchema::Thermal) &&
			context.payload.igniteStacks > 0)
		{
			const sas::GameplayEffectDefinition* igniteDefinition =
				EffectData::FindGameplayEffectDefinition("Effect.Status.Damage.Ignite");
			if (igniteDefinition)
			{
				sas::GameplayEffectSpec igniteSpec = MakeStatusEffectSpec(
					*igniteDefinition,
					GetDamageStatusBalance().thermal.duration,
					GetDamageStatusBalance().thermal.maxStacks
				);
				igniteSpec.attributes.clear();
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
