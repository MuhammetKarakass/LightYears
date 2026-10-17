#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/damage/DamageTypeSystem.h"

#include "gameConfigs/combat/CombatTick.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/targeting/CombatantTargetQuery.h"
#include "spaceShip/SpaceShip.h"
#include "player/PlayerSpaceShip.h"
#include "gameplay/content/DamageStatusBalanceCatalog.h"
#include "gameplay/effects/LightYearsEffectBehaviorRuntime.h"
#include "gameplay/module/generated/ModuleGeneratedDamage.h"
#include "gameplay/time/PeriodicTickAccumulator.h"
#include "effects/GameplayEffectBindings.h"
#include <algorithm>
#include <cmath>
#include <utility>

namespace vega
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

		ModuleStatusRules SourceStatusRules(const DamageContext& context)
		{
			if (context.payload.moduleTriggerPolicy == DamageModuleTriggerPolicy::Suppressed || context.payload.skipModuleDamageRules) return {};
			const auto* source = dynamic_cast<const Combatant*>(context.source);
			return source ? source->GetCombatRuntime().GetModuleStatusRules() : ModuleStatusRules{};
		}

		float StatusDurationMultiplier(const ModuleStatusRules& rules)
		{
			float multiplier = 1.f;
			for (const auto& rule : rules.durationBonuses) multiplier *= rule.durationMultiplier;
			return multiplier;
		}
		float StatusDurationBonus(const sas::AbilitySystemComponent& target, const ModuleStatusRules& rules)
		{
			const auto* thermal = target.FindGameplayEffectById(DamageStatusEffectIds::IgniteEffectId);
			const auto* cryo = target.FindGameplayEffectById(DamageStatusEffectIds::CryoSlowedEffectId);
			if (!thermal || thermal->stackCount <= 0 || !cryo || cryo->stackCount <= 0) return 0.f;
			float bonus = 0.f;
			for (const auto& rule : rules.thermalCryoDurationBonuses) bonus += rule.bonusSeconds;
			return bonus;
		}

		const ModuleElectricStatusRule* ElectricRule(const ModuleStatusRules& rules)
		{
			const ModuleElectricStatusRule* selected = nullptr;
			for (const auto& rule : rules.electricOverrides)
				if (!rule.damageTakenByStack.empty() && (!selected || rule.damageTakenByStack.size() > selected->damageTakenByStack.size() ||
					(rule.damageTakenByStack.size() == selected->damageTakenByStack.size() && rule.damageTakenByStack.back() > selected->damageTakenByStack.back()))) selected = &rule;
			return selected;
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
		bool ApplyCappedStackEffect(sas::AbilitySystemComponent& targetAbilitySystem, const sas::GameplayEffectSpec& spec, Actor* source, int incomingStacks)
		{
			bool applied = false;
			for (int stack = 0; stack < std::clamp(incomingStacks, 0, spec.maxStacks); ++stack)
			{
				applied = targetAbilitySystem.ApplyGameplayEffect(spec, source).IsValid() || applied;
			}
			return applied;
		}

		bool ApplyCryoStatus(sas::AbilitySystemComponent& targetAbilitySystem, const DamageContext& context, float durationBonus)
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
					GetDamageStatusBalance().cryo.duration + durationBonus,
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

		sas::GameplayEffectBehaviorResult TickIgnite(sas::ActiveGameplayEffect& effect, Actor& owner, float deltaTime)
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
				DamagePayload payload;
				if (sas::FindAttributeValue(effect.spec.attributes, DamageAttributeIds::SuppressModuleTriggers, 0.f) > 0.f)
				{
					payload.skipModuleDamageRules = true;
					payload.moduleTriggerPolicy = DamageModuleTriggerPolicy::Suppressed;
					payload.criticalPolicy = DamageCriticalPolicy::Disabled;
				}
				ApplyCombatDamage(
					owner,
					damage,
					effect.GetSourceObject<Actor>(),
					{ DamageTypeSchema::Thermal }, payload, {}, {}, DamageDeliveryType::Periodic
				);
			}
			return {};
		}

		sas::GameplayEffectBehaviorResult ProcessElectricIncomingDamage(sas::ActiveGameplayEffect& effect, DamageContext& context)
		{
			const auto* bonus = sas::FindAttribute(effect.spec.attributes, DamageAttributeIds::ElectricDamageTakenBonusForStack(effect.stackCount));
			context.remainingDamage *= 1.f + (bonus ? bonus->currentValue : GetDamageStatusBalance().ElectricDamageTakenMultiplier(effect.stackCount));
			return {};
		}

		bool ApplyKineticStatus(sas::AbilitySystemComponent& targetAbilitySystem, DamageContext& context, float durationBonus)
		{
			// Kinetic stacks shred the target's Armor for every incoming hit; that
			// is resolved by ResolveArmorShred, not by this hit's payload.
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
					GetDamageStatusBalance().kinetic.duration + durationBonus,
					GetDamageStatusBalance().kinetic.maxStacks
				),
				context.source,
				context.payload.kineticStacks
			);
		}
	}

	float DamageTypeSystem::ResolveArmorShred(
		const sas::AbilitySystemComponent& targetAbilitySystem
	)
	{
		const sas::ActiveGameplayEffect* kinetic = targetAbilitySystem.FindGameplayEffectById(
			DamageStatusEffectIds::KineticEffectId
		);
		if (!kinetic || kinetic->stackCount <= 0)
		{
			return 0.f;
		}
		return std::clamp(
			GetDamageStatusBalance().KineticArmorShred(kinetic->stackCount),
			0.f,
			1.f
		);
	}

	DamagePayload DamageTypeSystem::BuildPayload(const List<GameplayTag>& damageTags, const sas::GameplayAttributeList& sourceAttributes)
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
		payload.electricStacks = std::max(0, payload.electricStacks);
		payload.kineticStacks = std::clamp(
			payload.kineticStacks,
			0,
			GetDamageStatusBalance().kinetic.maxStacks
		);
		return payload;
	}

	namespace
	{
		// Electric, Cryo and Kinetic stacks stay while their source keeps
		// hitting the target with the same damage type: such a hit, with or
		// without new stacks, restarts the shared duration. Other damage types
		// from that source do not. When it runs out every stack clears at once
		// (the effects carry no per-stack decay).
		void RefreshSourceSustainedStatuses(sas::AbilitySystemComponent& targetAbilitySystem, const DamageContext& context, float durationBonus, float generalBonus, float durationMultiplier)
		{
			if (!context.source)
			{
				return;
			}
			const std::pair<GameplayTag, const char*> sustainedStatuses[] = {
				{ DamageTypeSchema::Electric, "Effect.Status.Damage.Electric" },
				{ DamageTypeSchema::Cryo, DamageStatusEffectIds::CryoSlowedEffectId },
				{ DamageTypeSchema::Kinetic, DamageStatusEffectIds::KineticEffectId }
			};
			for (const auto& [damageType, effectId] : sustainedStatuses)
			{
				if (!HasDamageType(context.damageTags, damageType))
				{
					continue;
				}
				sas::ActiveGameplayEffect* effect = targetAbilitySystem.FindGameplayEffectById(effectId);
				if (effect && effect->GetSourceObject<Actor>() == context.source)
				{
					if (damageType == DamageTypeSchema::Cryo) effect->spec.duration = GetDamageStatusBalance().cryo.duration + durationBonus;
					else if (damageType == DamageTypeSchema::Electric) effect->spec.duration = GetDamageStatusBalance().electric.duration + generalBonus;
					else effect->spec.duration = GetDamageStatusBalance().kinetic.duration + generalBonus;
					effect->spec.duration *= durationMultiplier;
					targetAbilitySystem.RefreshGameplayEffectDuration(effect->handle);
				}
			}
		}

		void SynchronizeElectricProfile(sas::AbilitySystemComponent& target, const ModuleElectricStatusRule* rule)
		{
			auto* effect = target.FindGameplayEffectById(DamageStatusEffectIds::ElectricEffectId);
			if (!effect || effect->stackCount <= 0) return;
			const int maxStacks = rule ? static_cast<int>(rule->damageTakenByStack.size()) : GetDamageStatusBalance().electric.maxStacks;
			effect->spec.maxStacks = maxStacks;
			effect->stackCount = std::min(effect->stackCount, maxStacks);
			effect->spec.attributes.clear();
			if (rule)
				for (int stack = 1; stack <= maxStacks; ++stack)
				{
					const float amount = rule->damageTakenByStack[static_cast<std::size_t>(stack - 1)];
					effect->spec.attributes.emplace_back(DamageAttributeIds::ElectricDamageTakenBonusForStack(stack), amount, amount);
				}
		}

		// Ember Accumulation. The owner's Thermal hit has just brought its Ignite stacks on this target to a rule threshold, so the
		// stacks are spent on one burst of damageFraction x the stack's full DoT (ThermalDamagePerSecond x thermal duration). The
		// pending DoT belongs to the spent stacks and is cancelled, not delivered. Area targets take areaDamageFraction of the burst
		// but get no stacks or DoT. The burst is generated damage, so it triggers no module rules.
		void ApplyThermalBurst(sas::AbilitySystemComponent& targetAbilitySystem, DamageContext& context, const ModuleStatusRules& rules)
		{
			if (rules.thermalBursts.empty() || context.payload.igniteStacks <= 0 || !HasDamageType(context.damageTags, DamageTypeSchema::Thermal) ||
				!context.source || !context.target) return;
			sas::ActiveGameplayEffect* ignite = targetAbilitySystem.FindGameplayEffectById(DamageStatusEffectIds::IgniteEffectId);
			if (!ignite || ignite->stackCount <= 0 || ignite->GetSourceObject<Actor>() != context.source) return;
			const ModuleThermalBurstRule* selected = nullptr;
			for (const auto& rule : rules.thermalBursts)
				if (ignite->stackCount >= rule.stackThreshold && (!selected || rule.stackThreshold < selected->stackThreshold)) selected = &rule;
			if (!selected) return;
			const ModuleThermalBurstRule rule = *selected;
			const float burst = std::max(0.f, rule.damageFraction) *
				GetDamageStatusBalance().ThermalDamagePerSecond(ignite->stackCount) * GetDamageStatusBalance().thermal.duration * StatusDurationMultiplier(rules);
			if (sas::GameplayAttribute* pendingDamage = sas::FindAttribute(ignite->runtimeAttributes, DamageAttributeIds::IgnitePendingDamage))
			{
				pendingDamage->currentValue = 0.f;
			}
			const auto handle = ignite->handle;
			targetAbilitySystem.RemoveGameplayEffect(handle);

			Actor* source = context.source;
			Actor* target = context.target;
			ApplyCombatDamage(*target, burst, source, { DamageTypeSchema::Thermal }, MakeModuleGeneratedDamagePayload(0, DamageSourceScalingPolicy::Normal), {}, {}, DamageDeliveryType::Area, nullptr);
			// Smoldering Remains: the field is owned by the source ship's reactive runtime. Its stacks use skipModuleDamageRules, so they
			// never reach this burst again.
			if (rule.smolderSeconds > 0.f)
			{
				if (auto* ship = dynamic_cast<PlayerSpaceShip*>(source)) ship->GetModuleApplier().SpawnSmolderingField(target->GetActorLocation(), rule);
			}
			if (rule.areaRadius <= 0.f || rule.areaDamageFraction <= 0.f || !target->GetWorld()) return;
			const float areaDamage = burst * std::clamp(rule.areaDamageFraction, 0.f, 1.f);
			const auto victims = targeting::FindOpposingCombatants(*target->GetWorld(), *source, target->GetActorLocation(), rule.areaRadius, false);
			for (const auto& victim : victims)
			{
				if (!victim || victim.get() == target || victim->GetIsPendingDestroy()) continue;
				ApplyCombatDamage(*victim, areaDamage, source, { DamageTypeSchema::Thermal }, MakeModuleGeneratedDamagePayload(0, DamageSourceScalingPolicy::Normal), {}, {}, DamageDeliveryType::Area, nullptr);
			}
		}

		// Kinetic Breaker. The owner's direct hit spends the target's Kinetic stacks once they reach the rule threshold (a Normal-class
		// target may use normalTargetThreshold). The hit then ignores the target's Armor and is multiplied; Armor Rend, when declared,
		// lowers the target's Armor for rendSeconds. Non-periodic hits spend stacks; generated damage has no source module rules.
		void ApplyKineticConsume(sas::AbilitySystemComponent& targetAbilitySystem, DamageContext& context, const ModuleStatusRules& rules)
		{
			if (rules.kineticConsumes.empty() || context.deliveryType == DamageDeliveryType::Periodic || !context.source || !context.target) return;
			sas::ActiveGameplayEffect* kinetic = targetAbilitySystem.FindGameplayEffectById(DamageStatusEffectIds::KineticEffectId);
			if (!kinetic || kinetic->stackCount <= 0 || kinetic->GetSourceObject<Actor>() != context.source) return;
			const auto* ship = dynamic_cast<const SpaceShip*>(context.target);
			const bool normalTarget = !ship || ship->GetControlTargetClass() == ControlTargetClass::Normal;
			const ModuleKineticConsumeRule* selected = nullptr;
			for (const auto& rule : rules.kineticConsumes)
			{
				const int threshold = normalTarget && rule.normalTargetThreshold > 0 ? rule.normalTargetThreshold : rule.stackThreshold;
				if (kinetic->stackCount >= threshold)
				{
					selected = &rule;
					break;
				}
			}
			if (!selected) return;
			const ModuleKineticConsumeRule rule = *selected;
			const auto handle = kinetic->handle;
			targetAbilitySystem.RemoveGameplayEffect(handle);
			// Penetration 1 makes the hull step ignore Armor completely (final multiplier 1), whatever the Kinetic shred was.
			context.payload.armorPenetration = 1.f;
			context.remainingDamage *= std::max(0.f, rule.damageMultiplier);
			if (rule.rendFraction <= 0.f || rule.rendSeconds <= 0.f) return;
			if (const sas::GameplayEffectDefinition* definition = EffectData::FindGameplayEffectDefinition(DamageStatusEffectIds::KineticArmorRendEffectId))
			{
				sas::GameplayEffectSpec spec = MakeStatusEffectSpec(*definition, rule.rendSeconds, 1);
				spec.modifiers = { sas::AttributeModifier{ OwnerAttributeIds::Armor, sas::AttributeModifierOperation::Multiply, 1.f - std::clamp(rule.rendFraction, 0.f, 1.f) } };
				targetAbilitySystem.ApplyGameplayEffect(spec, context.source);
			}
		}

		void ApplyPrimaryStatusPrimer(sas::AbilitySystemComponent& target, DamageContext& context, const ModuleStatusRules& rules)
		{
			if (rules.primaryStatusPrimers.empty() || !context.source || !context.target || context.deliveryType == DamageDeliveryType::Periodic ||
				!std::any_of(context.sourceAbilityTags.begin(), context.sourceAbilityTags.end(), [](const GameplayTag& tag) { return tag.MatchesTag(GameplayTags::Ability::Primary); })) return;
			const auto source = context.source->GetWeakActor().lock(), victim = context.target->GetWeakActor().lock();
			auto* combatant = source ? dynamic_cast<Combatant*>(source.get()) : nullptr;
			if (!source || !victim || !combatant) return;
			const sas::ActiveGameplayEffect* selected = nullptr;
			// Deterministic tie order; only damage statuses, never Frozen or its cooldown.
			for (const auto* id : { DamageStatusEffectIds::IgniteEffectId, DamageStatusEffectIds::CryoSlowedEffectId, DamageStatusEffectIds::ElectricEffectId, DamageStatusEffectIds::KineticEffectId })
				if (const auto* effect = target.FindGameplayEffectById(id); effect && effect->stackCount > 0 && (!selected || effect->stackCount > selected->stackCount)) selected = effect;
			if (!selected) return;
			const auto spec = selected->spec;
			for (const auto& rule : rules.primaryStatusPrimers)
				if (combatant->GetCombatRuntime().TryActivateModuleStatusPulse(rule.instanceId, rule.ruleIndex, victim->GetUniqueID(), rule.cooldownSeconds))
					ApplyCappedStackEffect(target, spec, source.get(), 1);
		}
		void ApplyElectricSpread(sas::AbilitySystemComponent& target, DamageContext& context, const ModuleStatusRules& rules)
		{
			if (rules.electricSpreads.empty() || !context.source || !context.target || !context.target->GetWorld()) return;
			const auto* electric = target.FindGameplayEffectById(DamageStatusEffectIds::ElectricEffectId);
			if (!electric || electric->GetSourceObject<Actor>() != context.source) return;
			const auto source = context.source->GetWeakActor().lock();
			const auto original = context.target->GetWeakActor().lock();
			auto* combatant = source ? dynamic_cast<Combatant*>(source.get()) : nullptr;
			if (!source || !original || !combatant) return;
			const int stackCount = electric->stackCount;
			for (const auto& rule : rules.electricSpreads)
			{
				if (stackCount < rule.threshold || !combatant->GetCombatRuntime().TryActivateElectricSpread(rule, original->GetUniqueID())) continue;
				auto targets = targeting::FindOpposingCombatants(*original->GetWorld(), *source, original->GetActorLocation(), rule.radius, false);
				const auto origin = original->GetActorLocation();
				std::sort(targets.begin(), targets.end(), [origin](const auto& a, const auto& b)
				{
					const auto da = a->GetActorLocation() - origin, db = b->GetActorLocation() - origin;
					const float left = da.x * da.x + da.y * da.y, right = db.x * db.x + db.y * db.y;
					return left == right ? a->GetUniqueID() < b->GetUniqueID() : left < right;
				});
				int count = 0;
				for (const auto& actor : targets)
				{
					if (actor == original || actor->GetIsPendingDestroy()) continue;
					auto* victim = dynamic_cast<Combatant*>(actor.get());
					if (!victim) continue;
					DamageContext spreadContext;
					spreadContext.source = source.get(); spreadContext.target = actor.get(); spreadContext.remainingDamage = 1.f;
					spreadContext.damageTags = { DamageTypeSchema::Electric };
					spreadContext.payload.electricStacks = rule.stacks;
					spreadContext.payload.kineticStacks = 0;
					spreadContext.payload.moduleTriggerPolicy = DamageModuleTriggerPolicy::Suppressed;
					DamageTypeSystem::ApplyStatusEffects(victim->GetAbilitySystemComponent(), spreadContext);
					if (++count >= rule.targetCount || source->GetIsPendingDestroy() || original->GetIsPendingDestroy()) break;
				}
			}
		}
		void ApplyCryoFreeze(sas::AbilitySystemComponent& target, DamageContext& context, const ModuleStatusRules& rules, float durationBonus)
		{
			if (target.FindGameplayEffectById(DamageStatusEffectIds::CryoFreezeCooldownEffectId)) return;
			const auto* slowed = target.FindGameplayEffectById(DamageStatusEffectIds::CryoSlowedEffectId);
			if (!slowed) return;
			const auto* ship = dynamic_cast<const SpaceShip*>(context.target);
			const ModuleCryoFreezeRule* selected = nullptr;
			for (const auto& rule : rules.cryoFreezes)
			{
				if (rule.threshold <= 0 || slowed->stackCount < rule.threshold ||
					(rule.normalTargetsOnly && ship && ship->GetControlTargetClass() != ControlTargetClass::Normal)) continue;
				if (!selected || rule.durationSeconds > selected->durationSeconds) selected = &rule;
			}
			if (!selected) return;
			const auto* definition = EffectData::FindGameplayEffectDefinition(DamageStatusEffectIds::CryoFrozenEffectId);
			const auto* cooldownDefinition = EffectData::FindGameplayEffectDefinition(DamageStatusEffectIds::CryoFreezeCooldownEffectId);
			if (!definition || !cooldownDefinition) return;
			const auto sourceLifetime = context.source ? context.source->GetWeakActor().lock() : std::shared_ptr<Actor>{};
			const auto targetLifetime = context.target ? context.target->GetWeakActor().lock() : std::shared_ptr<Actor>{};
			const auto slowHandle = slowed->handle;
			const auto frozen = target.ApplyGameplayEffect(MakeStatusEffectSpec(*definition, (selected->durationSeconds + durationBonus) * StatusDurationMultiplier(rules), 1), context.source);
			if (!frozen.IsValid()) return;
			target.ApplyGameplayEffect(MakeStatusEffectSpec(*cooldownDefinition, selected->cooldownSeconds, 1), context.source);
			target.RemoveGameplayEffect(slowHandle);
			if (!sourceLifetime || !targetLifetime || !context.target->GetWorld()) return;
			for (const auto& spread : rules.freezeSpreads)
			{
				const auto targets = targeting::FindOpposingCombatants(*context.target->GetWorld(), *sourceLifetime, context.target->GetActorLocation(), spread.radius, false);
				for (const auto& actor : targets)
				{
					if (actor == targetLifetime || actor->GetIsPendingDestroy()) continue;
					auto* combatant = dynamic_cast<Combatant*>(actor.get());
					if (!combatant) continue;
					DamageContext spreadContext;
					spreadContext.source = sourceLifetime.get();
					spreadContext.target = actor.get();
					spreadContext.remainingDamage = 1.f; // Status only; never resolves a damage hit.
					spreadContext.damageTags = { DamageTypeSchema::Cryo };
					spreadContext.payload.cryoBuildupPerHit = spread.stacks;
					spreadContext.payload.moduleTriggerPolicy = DamageModuleTriggerPolicy::Suppressed;
					DamageTypeSystem::ApplyStatusEffects(combatant->GetAbilitySystemComponent(), spreadContext);
				}
			}
		}
	}

	List<GameplayTag> DamageTypeSystem::ApplyStatusEffects(sas::AbilitySystemComponent& targetAbilitySystem, DamageContext& context)
	{
		List<GameplayTag> applied;
		if (context.remainingDamage <= 0.f)
		{
			return applied;
		}
		const ModuleStatusRules rules = SourceStatusRules(context);
		ApplyPrimaryStatusPrimer(targetAbilitySystem, context, rules);
		float generalBonus = 0.f;
		for (const auto& rule : rules.durationBonuses) generalBonus += rule.bonusSeconds;
		const float durationBonus = generalBonus + StatusDurationBonus(targetAbilitySystem, rules);
		const float durationMultiplier = StatusDurationMultiplier(rules);
		const auto* electricRule = ElectricRule(rules);
		if (HasDamageType(context.damageTags, DamageTypeSchema::Electric) && context.payload.electricStacks == 0)
		{
			const auto* existing = targetAbilitySystem.FindGameplayEffectById(DamageStatusEffectIds::ElectricEffectId);
			if (existing && existing->GetSourceObject<Actor>() == context.source) SynchronizeElectricProfile(targetAbilitySystem, electricRule);
		}
		ApplyKineticConsume(targetAbilitySystem, context, rules);
		RefreshSourceSustainedStatuses(targetAbilitySystem, context, durationBonus, generalBonus, durationMultiplier);

		if (HasDamageType(context.damageTags, DamageTypeSchema::Thermal) &&
			context.payload.igniteStacks > 0)
		{
			const sas::GameplayEffectDefinition* igniteDefinition =
				EffectData::FindGameplayEffectDefinition("Effect.Status.Damage.Ignite");
			if (igniteDefinition)
			{
				sas::GameplayEffectSpec igniteSpec = MakeStatusEffectSpec(
					*igniteDefinition,
					(GetDamageStatusBalance().thermal.duration + durationBonus) * durationMultiplier,
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
		ApplyThermalBurst(targetAbilitySystem, context, rules);
		if (HasDamageType(context.damageTags, DamageTypeSchema::Cryo) &&
			context.payload.cryoBuildupPerHit > 0)
		{
			if (ApplyCryoStatus(targetAbilitySystem, context, (GetDamageStatusBalance().cryo.duration + durationBonus) * durationMultiplier - GetDamageStatusBalance().cryo.duration))
			{
				applied.push_back(DamageStatusSchema::CryoSlowed);
				ApplyCryoFreeze(targetAbilitySystem, context, rules, generalBonus);
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
					(GetDamageStatusBalance().electric.duration + generalBonus) * durationMultiplier,
					electricRule ? static_cast<int>(electricRule->damageTakenByStack.size()) : GetDamageStatusBalance().electric.maxStacks
				);
				electricSpec.attributes.clear();
				if (electricRule)
					for (std::size_t stack = 0; stack < electricRule->damageTakenByStack.size(); ++stack)
					{
						const float amount = electricRule->damageTakenByStack[stack];
						electricSpec.attributes.emplace_back(DamageAttributeIds::ElectricDamageTakenBonusForStack(static_cast<int>(stack + 1)), amount, amount);
					}
				if (ApplyCappedStackEffect(
					targetAbilitySystem,
					electricSpec,
					context.source,
					context.payload.electricStacks
				))
				{
					applied.push_back(DamageStatusSchema::Electric);
					ApplyElectricSpread(targetAbilitySystem, context, rules);
				}
			}
		}
		if (HasDamageType(context.damageTags, DamageTypeSchema::Kinetic) &&
			ApplyKineticStatus(targetAbilitySystem, context, (GetDamageStatusBalance().kinetic.duration + generalBonus) * durationMultiplier - GetDamageStatusBalance().kinetic.duration))
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
