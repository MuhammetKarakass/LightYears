#include "gameplay/ability/astralSurge/AstralSurgeAbility.h"

#include "gameplay/ability/runtime/FocusActionLocks.h"

#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/actors/AbilityActorSpawner.h"
#include "gameplay/ability/astralSurge/AstralSurgeContracts.h"
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/astralSurge/AstralSurgePresentationIds.h"
#include "presentation/ability/astralSurge/AstralSurgePresentationProfile.h"
#include "framework/World.h"
#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		sf::Vector2f ResolveMouseDirection(const Actor& owner)
		{
			if (const World* world = owner.GetWorld(); world && world->GetApplication())
			{
				sf::Vector2f direction = world->GetMouseWorldPosition() -
					owner.GetActorLocation();
				if (GetVectorLength(direction) > 0.001f)
				{
					NormalizeVector(direction);
					return direction;
				}
			}
			return { 0.f, -1.f };
		}

		bool HasExpectedScalingRule(const GameAbilityDefinition& definition)
		{
			if (definition.scalingRules.size() != 1)
			{
				return false;
			}
			const sas::AttributeScalingRule& rule = definition.scalingRules.front();
			return rule.targetAttributeId == CommonAttributeIds::Damage &&
				rule.sourceAttributeId == OwnerAttributeIds::EnergyPower &&
				rule.operation == sas::AttributeModifierOperation::Add &&
				std::isfinite(rule.coefficient) && rule.coefficient > 0.f;
		}

		bool HasExpectedLevelStep(const AbilityLevelStep& step)
		{
			int damageModifiers = 0;
			for (const sas::AttributeModifier& modifier : step.attributeModifiers)
			{
				if (modifier.attributeId == CommonAttributeIds::Damage &&
					modifier.operation == sas::AttributeModifierOperation::Add &&
					std::isfinite(modifier.magnitude) && modifier.magnitude > 0.f)
				{
					++damageModifiers;
				}
				else
				{
					return false;
				}
			}
			if (damageModifiers != 1 ||
				step.scalingRules.size() != 1)
			{
				return false;
			}
			const sas::AttributeScalingRule& scalingRule = step.scalingRules.front();
			return scalingRule.targetAttributeId == CommonAttributeIds::Damage &&
				scalingRule.sourceAttributeId == OwnerAttributeIds::EnergyPower &&
				scalingRule.operation == sas::AttributeModifierOperation::Add &&
				std::isfinite(scalingRule.coefficient) && scalingRule.coefficient > 0.f;
		}
	}

	bool AstralSurgeAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		const bool validLifecycle =
			definition.abilityId == AbilityData::AstralSurge::AbilityId::Basic &&
			sas::IsLoadoutAbilitySlot(definition.slot) &&
			definition.activationPolicy == sas::AbilityActivationPolicy::OnPressed &&
			definition.lifetimePolicy == sas::AbilityLifetimePolicy::Duration &&
			definition.maxCharges == 1 && definition.cooldown > 0.f &&
			definition.duration > 0.f;
		if (!validLifecycle || !HasExpectedScalingRule(definition))
		{
			if (failureReason)
			{
				*failureReason =
					"Astral Surge requires a timed OnPressed lifecycle and positive additive EnergyPower damage scaling.";
			}
			return false;
		}

		const AbilityActorDefinition* actorDefinition =
			AbilityData::FindAbilityActorDefinition(
				AbilityData::AstralSurge::Actor::Projectile::BasicDefinitionId
			);
		if (!actorDefinition)
		{
			if (failureReason)
			{
				*failureReason = "Astral Surge projectile definition is missing.";
			}
			return false;
		}
		for (const sas::AttributeId& required : {
			CommonAttributeIds::Damage,
			CommonAttributeIds::PierceDamageLoss,
			AreaAttributeIds::Width,
			AbilityData::AstralSurge::Actor::Projectile::ProjectileSpeed,
			AbilityData::AstralSurge::Actor::Projectile::MinimumDamageMultiplier
		})
		{
			if (!sas::FindAttribute(actorDefinition->attributes, required))
			{
				if (failureReason)
				{
					*failureReason = "Astral Surge must declare every projectile runtime attribute.";
				}
				return false;
			}
		}

		const auto value = [&](const sas::AttributeId& id)
		{
			return sas::FindAttributeValue(actorDefinition->attributes, id, 0.f);
		};
		if (value(AreaAttributeIds::Width) <= 0.f ||
			value(AbilityData::AstralSurge::Actor::Projectile::ProjectileSpeed) <= 0.f ||
			value(CommonAttributeIds::PierceDamageLoss) < 0.f ||
			value(CommonAttributeIds::PierceDamageLoss) >= 1.f ||
			value(AbilityData::AstralSurge::Actor::Projectile::MinimumDamageMultiplier) < 0.f ||
			value(AbilityData::AstralSurge::Actor::Projectile::MinimumDamageMultiplier) > 1.f)
		{
			if (failureReason)
			{
				*failureReason = "Astral Surge contains invalid projectile balance values.";
			}
			return false;
		}

		if (definition.damageTags != List<GameplayTag>{ DamageTypeSchema::Energy } ||
			(definition.levelProgression.empty() && definition.repeatingLevelProgression.empty()) ||
			!std::all_of(
				definition.levelProgression.begin(),
				definition.levelProgression.end(),
				HasExpectedLevelStep
			) ||
			!std::all_of(
				definition.repeatingLevelProgression.begin(),
				definition.repeatingLevelProgression.end(),
				HasExpectedLevelStep
			))
		{
			if (failureReason)
			{
				*failureReason = "Astral Surge requires Energy damage and fourteen positive Damage/EnergyPower progression steps.";
			}
			return false;
		}

		if (!PresentationProfileRegistry<AstralSurgePresentationProfile>::Find(
			AstralSurgePresentationIds::ProjectileBasic
		))
		{
			if (failureReason)
			{
				*failureReason = "Astral Surge presentation profile is not registered.";
			}
			return false;
		}
		return true;
	}

	bool AstralSurgeAbility::Activate(GameAbilityBehaviorContext& context)
	{
		if (mStarted)
		{
			return false;
		}

		mStarted = true;
		mFired = false;
		mFocusElapsed = 0.f;
		mDirection = ResolveMouseDirection(context.owner);

		// Astral Surge is a committed focus: input, primary fire, and every
		// other ability are blocked until the one-second wave release completes.
		ability::ApplyFocusActionLocks(context.abilitySystem);
		context.abilitySystem.AddOwnedTag(AbilityData::AstralSurge::State::Focusing);

		EmitEvent(context, AbilityData::AstralSurge::Event::Started);
		return true;
	}

	void AstralSurgeAbility::Tick(
		GameAbilityBehaviorContext& context,
		float deltaTime
	)
	{
		if (!mStarted || mFired)
		{
			return;
		}

		mDirection = ResolveMouseDirection(context.owner);
		mFocusElapsed = std::min(
			std::max(0.f, context.definition.duration),
			mFocusElapsed + std::max(0.f, deltaTime)
		);
		const float duration = std::max(0.001f, context.definition.duration);
		const float progress = std::clamp(mFocusElapsed / duration, 0.f, 1.f);

		if (progress >= 1.f)
		{
			Fire(context);
			context.instance.Cancel(sas::AbilityEndReason::Completed);
		}
	}

	void AstralSurgeAbility::End(
		GameAbilityBehaviorContext& context,
		sas::AbilityEndReason reason
	)
	{
		(void)reason;
		if (!mStarted)
		{
			return;
		}

		RemoveFocusLocks(context);
		EmitEvent(context, AbilityData::AstralSurge::Event::Ended);
		mFocusElapsed = 0.f;
		mDirection = { 0.f, -1.f };
		mStarted = false;
		mFired = false;
	}

	void AstralSurgeAbility::Fire(GameAbilityBehaviorContext& context)
	{
		if (mFired)
		{
			return;
		}
		mFired = true;

		AbilityExecutionContext executionContext{
			&context.abilitySystem,
			&context.definition,
			nullptr,
			&context.instance
		};
		const AbilityActorDefinition* actorDefinition =
			AbilityData::FindAbilityActorDefinition(
				AbilityData::AstralSurge::Actor::Projectile::BasicDefinitionId
			);
		const float waveWidth = actorDefinition
			? std::max(1.f, sas::FindAttributeValue(
				actorDefinition->attributes, AreaAttributeIds::Width, 280.f
			))
			: 280.f;
		// The wave begins in front of its owner. Its wide damage band is swept
		// separately, so spawning it ahead prevents an accidental radial hit at
		// the ship while retaining a broad forward-facing attack.
		const sf::Vector2f spawnLocation = context.owner.GetActorLocation() +
			mDirection * (waveWidth * 0.5f + 40.f);
		AbilityActorSpawner::SpawnAtLocation(
			AbilityData::AstralSurge::Actor::Projectile::BasicDefinitionId,
			executionContext,
			context.owner,
			spawnLocation,
			mDirection
		);
		EmitEvent(context, AbilityData::AstralSurge::Event::Fired);
	}

	void AstralSurgeAbility::RemoveFocusLocks(
		GameAbilityBehaviorContext& context
	) const
	{
		ability::RemoveFocusActionLocks(context.abilitySystem);
		context.abilitySystem.RemoveOwnedTag(AbilityData::AstralSurge::State::Focusing);
	}

	void AstralSurgeAbility::EmitEvent(
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
