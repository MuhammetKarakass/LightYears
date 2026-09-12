#include "gameplay/ability/nullPulse/NullPulseAbility.h"

#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameplay/ability/nullPulse/NullPulseContracts.h"
#include "gameplay/ability/nullPulse/NullPulseTargetQuery.h"
#include "gameplay/ability/nullPulse/NullPulseVisualActor.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/content/EffectContentCatalog.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/nullPulse/NullPulsePresentationIds.h"
#include "presentation/ability/nullPulse/NullPulsePresentationProfile.h"
#include "framework/Actor.h"
#include "framework/World.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		const sas::GameplayAttribute* FindAttribute(
			const GameAbilityDefinition& definition,
			const sas::AttributeId& attributeId
		)
		{
			return sas::FindAttribute(definition.attributes, attributeId);
		}

		bool IsFiniteNonNegative(float value)
		{
			return std::isfinite(value) && value >= 0.f;
		}

		float FindValue(
			const sas::GameplayAttributeList& values,
			const sas::AttributeId& attributeId,
			float fallback
		)
		{
			return sas::FindAttributeValue(values, attributeId, fallback);
		}

		float ResolveStunDuration(
			const sas::GameplayAttributeList& values,
			const Actor& owner
		)
		{
			const float baseDuration = std::max(
				0.f,
				FindValue(
					values,
					AbilityData::NullPulse::Attribute::BaseStunDuration,
					0.f
				)
			);
			const float maximumBonus = std::max(
				0.f,
				FindValue(
					values,
					AbilityData::NullPulse::Attribute::MaxBonusStun,
					0.f
				)
			);
			const float referenceEnergy = std::max(
				0.f,
				FindValue(
					values,
					AbilityData::NullPulse::Attribute::ReferenceEnergyPower,
					0.f
				)
			);
			const float energyScale = std::max(
				0.001f,
				FindValue(
					values,
					AbilityData::NullPulse::Attribute::EnergyScale,
					1.f
				)
			);

			const auto* sourceCombatant = dynamic_cast<const Combatant*>(&owner);
			const float resolvedEnergyPower = sourceCombatant
				? sourceCombatant->GetAbilitySystemComponent().GetAttributes().GetCurrentValue(
					OwnerAttributeIds::EnergyPower
				)
				: referenceEnergy;
			const float bonusEnergy = std::max(0.f, resolvedEnergyPower - referenceEnergy);
			const float bonusDuration = maximumBonus * (
				1.f - std::exp(-bonusEnergy / energyScale)
			);
			return std::max(0.f, baseDuration + bonusDuration);
		}

		bool ApplyControlEffect(
			Combatant& target,
			Actor& source,
			const GameAbilityDefinition& abilityDefinition,
			const GameAbility& ability,
			const sas::GameplayEffectDefinition& effectDefinition,
			float duration,
			float controlMultiplier,
			const sas::ContentId& presentationProfileId
		)
		{
			if (duration <= 0.f)
			{
				return false;
			}

			sas::GameplayEffectSpec spec = sas::MakeGameplayEffectSpec(effectDefinition);
			spec.duration = duration;
			spec.maxStacks = 1;
			auto runtimeContext = std::make_shared<NullPulseControlRuntimeContext>();
			runtimeContext->sourceAbilityId = sas::ContentId{ abilityDefinition.abilityId };
			runtimeContext->sourceAbilityTags = abilityDefinition.abilityTags;
			runtimeContext->targetControlMultiplier = controlMultiplier;
			runtimeContext->presentationProfileId = presentationProfileId;

			return target.GetAbilitySystemComponent().ApplyGameplayEffect(
				spec,
				sas::GameplayEffectSourceContext{ &source, &ability, runtimeContext }
			).IsValid();
		}
	}

	bool NullPulseAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		if (definition.abilityId != AbilityData::NullPulse::AbilityId::Basic ||
			!sas::IsLoadoutAbilitySlot(definition.slot) ||
			definition.activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
			definition.lifetimePolicy != sas::AbilityLifetimePolicy::Instant ||
			definition.maxCharges != 1 || definition.cooldown <= 0.f ||
			definition.duration != 0.f)
		{
			if (failureReason)
			{
				*failureReason =
					"Null Pulse requires a loadout slot, pressed activation, and one charge.";
			}
			return false;
		}

		for (const sas::AttributeId& required : {
			AbilityData::NullPulse::Attribute::Radius,
			AbilityData::NullPulse::Attribute::Damage,
			AbilityData::NullPulse::Attribute::BaseStunDuration,
			AbilityData::NullPulse::Attribute::MaxBonusStun,
			AbilityData::NullPulse::Attribute::ReferenceEnergyPower,
			AbilityData::NullPulse::Attribute::EnergyScale,
			AbilityData::NullPulse::Attribute::BossStaggerDuration
		})
		{
			const sas::GameplayAttribute* attribute = FindAttribute(definition, required);
			if (!attribute || !std::isfinite(attribute->baseValue))
			{
				if (failureReason)
				{
					*failureReason =
						"Null Pulse must declare all runtime control attributes.";
				}
				return false;
			}
		}

		if (FindAttribute(definition, AbilityData::NullPulse::Attribute::Radius)->baseValue <= 0.f ||
			!IsFiniteNonNegative(FindAttribute(definition, AbilityData::NullPulse::Attribute::Damage)->baseValue) ||
			FindAttribute(definition, AbilityData::NullPulse::Attribute::BaseStunDuration)->baseValue <= 0.f ||
			!IsFiniteNonNegative(FindAttribute(definition, AbilityData::NullPulse::Attribute::MaxBonusStun)->baseValue) ||
			!IsFiniteNonNegative(FindAttribute(definition, AbilityData::NullPulse::Attribute::ReferenceEnergyPower)->baseValue) ||
			FindAttribute(definition, AbilityData::NullPulse::Attribute::EnergyScale)->baseValue <= 0.f ||
			!IsFiniteNonNegative(FindAttribute(definition, AbilityData::NullPulse::Attribute::BossStaggerDuration)->baseValue))
		{
			if (failureReason)
			{
				*failureReason = "Null Pulse has invalid radius, damage, stun or energy values.";
			}
			return false;
		}

		const sas::GameplayEffectDefinition* stunDefinition =
			EffectData::FindGameplayEffectDefinition(AbilityData::NullPulse::Effect::StunId);
		const sas::GameplayEffectDefinition* staggerDefinition =
			EffectData::FindGameplayEffectDefinition(AbilityData::NullPulse::Effect::StaggerId);
		if (!stunDefinition || !staggerDefinition ||
			stunDefinition->durationPolicy != sas::GameplayEffectDurationPolicy::Duration ||
			staggerDefinition->durationPolicy != sas::GameplayEffectDurationPolicy::Duration)
		{
			if (failureReason)
			{
				*failureReason = "Null Pulse requires reusable Stun and Stagger effects.";
			}
			return false;
		}

		return true;
	}

	bool NullPulseAbility::Activate(GameAbilityBehaviorContext& context)
	{
		AbilityExecutionContext executionContext{
			&context.abilitySystem,
			&context.definition,
			nullptr,
			&context.instance
		};
		const sas::GameplayAttributeList values =
			AbilityActionAttributeResolver::ResolveAbilityAttributes(executionContext);
		const float radius = std::max(
			0.f,
			FindValue(values, AbilityData::NullPulse::Attribute::Radius, 0.f)
		);
		const float damage = std::max(
			0.f,
			FindValue(values, AbilityData::NullPulse::Attribute::Damage, 0.f)
		);
		if (radius <= 0.f || damage <= 0.f)
		{
			return false;
		}

		// Presentation is optional at runtime, but when the typed profile is
		// registered the pulse gets one feature-local visual actor. The actor has
		// no collision and cannot affect the gameplay query below.
		if (World* world = context.owner.GetWorld())
		{
			if (const NullPulsePresentationProfile* profile =
				PresentationProfileRegistry<NullPulsePresentationProfile>::Find(
					NullPulsePresentationIds::PulseBasic
				))
			{
				world->SpawnActor<NullPulseVisualActor>(
					context.owner.GetActorLocation(),
					radius,
					*profile
				);
			}
		}

		const List<shared_ptr<AbilityWorldActor>> projectiles =
			NullPulseTargetQuery::FindClearableProjectiles(
				*context.owner.GetWorld(), context.owner, radius
			);
		for (const shared_ptr<AbilityWorldActor>& projectile : projectiles)
		{
			if (projectile)
			{
				projectile->Destroy();
			}
		}
		EmitEvent(context, AbilityData::NullPulse::Event::Activated);
		if (!projectiles.empty())
		{
			EmitEvent(context, AbilityData::NullPulse::Event::ProjectilesCleared);
		}

		const List<shared_ptr<Actor>> targets =
			NullPulseTargetQuery::FindEnemyTargets(
				*context.owner.GetWorld(), context.owner, radius
			);
		const List<GameplayTag> damageTags =
			context.instance.GetResolvedDamageTags(AttachmentHostKind::Ability);
		DamagePayload payload = DamageTypeSystem::BuildPayload(damageTags);
		payload.criticalPolicy = DamageCriticalPolicy::Disabled;

		const sas::GameplayEffectDefinition* stunDefinition =
			EffectData::FindGameplayEffectDefinition(AbilityData::NullPulse::Effect::StunId);
		const sas::GameplayEffectDefinition* staggerDefinition =
			EffectData::FindGameplayEffectDefinition(AbilityData::NullPulse::Effect::StaggerId);
		const float baseStunDuration = ResolveStunDuration(values, context.owner);
		const float bossStaggerDuration = std::max(
			0.f,
			FindValue(values, AbilityData::NullPulse::Attribute::BossStaggerDuration, 0.f)
		);

		for (const shared_ptr<Actor>& target : targets)
		{
			if (!target)
			{
				continue;
			}
			ApplyCombatDamage(
				*target,
				damage,
				&context.owner,
				damageTags,
				payload,
				sas::ContentId{ context.definition.abilityId },
				context.definition.abilityTags
			);

			Combatant* combatant = dynamic_cast<Combatant*>(target.get());
			if (!combatant)
			{
				continue;
			}
			const ControlResponse response = combatant->ResolveControlResponse(
				GameplayTags::State::Effect::Control::Stunned
			);
			if (response.mode == ControlResponseMode::Immune)
			{
				continue;
			}
			if (response.mode == ControlResponseMode::InterruptOnly)
			{
				if (staggerDefinition && response.interruptionAllowed)
				{
					ApplyControlEffect(
						*combatant,
						context.owner,
						context.definition,
						context.instance,
						*staggerDefinition,
						std::min(bossStaggerDuration, response.maximumInterruptDuration),
						0.f,
						sas::ContentId{ NullPulsePresentationIds::PulseBasic }
					);
				}
				continue;
			}
			if (stunDefinition)
			{
				ApplyControlEffect(
					*combatant,
					context.owner,
					context.definition,
					context.instance,
					*stunDefinition,
					baseStunDuration * std::max(0.f, response.durationMultiplier),
					response.durationMultiplier,
					sas::ContentId{ NullPulsePresentationIds::PulseBasic }
				);
			}
		}

		EmitEvent(context, AbilityData::NullPulse::Event::Completed);
		return true;
	}

	void NullPulseAbility::EmitEvent(
		GameAbilityBehaviorContext& context,
		const GameplayTag& eventTag
	) const
	{
		sas::AbilityEvent event;
		event.eventTag = eventTag;
		event.sourceAbilityId = sas::ContentId{ context.definition.abilityId };
		event.sourceAbilityTags = context.definition.abilityTags;
		event.SetSource(&context.owner);
		event.SetTarget(&context.owner);
		context.abilitySystem.HandleGameplayEvent(event);
	}
}
