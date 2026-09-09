#include "gameplay/ability/foldspaceArena/FoldspaceArenaAbility.h"

#include "attributes/AttributeSystem.h"
#include "framework/World.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/actors/AbilityActorSpawner.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/ability/foldspaceArena/FoldspaceArenaActor.h"
#include "gameplay/ability/foldspaceArena/FoldspaceArenaContracts.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		constexpr float Epsilon = 0.001f;

		bool NearlyEqual(float left, float right)
		{
			return std::isfinite(left) && std::isfinite(right) &&
				std::abs(left - right) <= Epsilon;
		}

		bool HasExpectedLevelStep(const AbilityLevelStep& step)
		{
			if (step.attributeModifiers.size() != 3 ||
				!step.unlockedUpgradeIds.empty() ||
				!step.addedActions.empty() ||
				!step.addedTriggers.empty())
			{
				return false;
			}

			bool damageStep = false;
			bool durationStep = false;
			bool cooldownStep = false;
			for (const sas::AttributeModifier& modifier : step.attributeModifiers)
			{
				if (modifier.attributeId == CommonAttributeIds::Damage &&
					modifier.operation == sas::AttributeModifierOperation::Add &&
					NearlyEqual(modifier.magnitude, 2.f))
				{
					damageStep = true;
				}
				else if (modifier.attributeId ==
					AbilityData::FoldspaceArena::Attribute::BaseArenaDuration &&
					modifier.operation == sas::AttributeModifierOperation::Add &&
					NearlyEqual(modifier.magnitude, 0.10f))
				{
					durationStep = true;
				}
				else if (modifier.attributeId == CommonAttributeIds::Cooldown &&
					modifier.operation == sas::AttributeModifierOperation::Add &&
					NearlyEqual(modifier.magnitude, -0.25f))
				{
					cooldownStep = true;
				}
			}
			return damageStep && durationStep && cooldownStep;
		}

		bool HasPositiveAttribute(
			const GameAbilityDefinition& definition,
			const sas::AttributeId& id
		)
		{
			const sas::GameplayAttribute* attribute = sas::FindAttribute(
				definition.attributes, id
			);
			return attribute && std::isfinite(attribute->baseValue) &&
				attribute->baseValue > 0.f;
		}

		sas::GameplayAttributeList ResolveValues(
			GameAbilityBehaviorContext& context
		)
		{
			return AbilityActionAttributeResolver::ResolveAbilityAttributes(
				AbilityExecutionContext{
					&context.abilitySystem,
					&context.definition,
					nullptr,
					&context.instance
				}
			);
		}
	}

	bool FoldspaceArenaAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		const AbilityActorDefinition* arena = AbilityData::FindAbilityActorDefinition(
			AbilityData::FoldspaceArena::Actor::Arena::BasicDefinitionId
		);
		const bool validIdentity =
			definition.abilityId == AbilityData::FoldspaceArena::AbilityId::Basic &&
			definition.behaviorType == AbilityBehaviorType::FoldspaceArena &&
			definition.abilityTags.size() == 2 &&
			std::any_of(definition.abilityTags.begin(), definition.abilityTags.end(),
				[](const GameplayTag& tag)
				{
					return tag.MatchesTagExact(AbilityData::FoldspaceArena::CategoryTag);
				}) &&
			std::any_of(definition.abilityTags.begin(), definition.abilityTags.end(),
				[](const GameplayTag& tag)
				{
					return tag.MatchesTagExact(AbilityData::FoldspaceArena::FamilyTag);
				});
		const bool validLifecycle = sas::IsLoadoutAbilitySlot(definition.slot) &&
			definition.activationPolicy == sas::AbilityActivationPolicy::OnPressed &&
			definition.lifetimePolicy == sas::AbilityLifetimePolicy::Duration &&
			definition.maxCharges == 1 && definition.duration >= 7.f &&
			definition.cooldown > 0.f;
		const List<sas::AttributeId> requiredAttributes{
			CommonAttributeIds::Range,
			CommonAttributeIds::Damage,
			AbilityData::FoldspaceArena::Attribute::MinimumArenaDuration,
			AbilityData::FoldspaceArena::Attribute::BaseArenaDuration,
			AbilityData::FoldspaceArena::Attribute::EnergyMaxDurationReference,
			AbilityData::FoldspaceArena::Attribute::EnergyMaxDurationPerPoint
		};
		const bool validAttributes = std::all_of(
			requiredAttributes.begin(), requiredAttributes.end(),
			[&definition](const sas::AttributeId& id)
			{
				return HasPositiveAttribute(definition, id);
			}
		);
		const bool validDamage = definition.damageTags.size() == 1 &&
			definition.damageTags.front().MatchesTagExact(DamageTypeSchema::Photonic);
		const bool validScaling = definition.scalingRules.size() == 1 &&
			definition.scalingRules.front().targetAttributeId == CommonAttributeIds::Damage &&
			definition.scalingRules.front().sourceAttributeId == OwnerAttributeIds::EnergyMax &&
			definition.scalingRules.front().operation == sas::AttributeModifierOperation::Add &&
			NearlyEqual(definition.scalingRules.front().coefficient, 0.08f);
		const bool validProgression = definition.levelProgression.size() == 14 &&
			std::all_of(
				definition.levelProgression.begin(),
				definition.levelProgression.end(),
				HasExpectedLevelStep
			);
		const bool validActor = arena &&
			arena->actorType == AbilityActorType::FoldspaceArena &&
			arena->lifeTime >= definition.duration && arena->presentationProfileId.IsValid();

		if (!validIdentity || !validLifecycle || !validAttributes || !validDamage ||
			!validScaling || !validProgression || !validActor ||
			!definition.actions.empty())
		{
			if (failureReason)
			{
				*failureReason =
					"Foldspace Arena requires its duration lifecycle, Photonic formation damage, and rounded-arena runtime attributes.";
			}
			return false;
		}
		return true;
	}

	bool FoldspaceArenaAbility::Activate(GameAbilityBehaviorContext& context)
	{
		World* world = context.owner.GetWorld();
		if (!world || mActive)
		{
			return false;
		}

		const sas::GameplayAttributeList values = ResolveValues(context);
		const float castRange = std::max(1.f, sas::FindAttributeValue(
			values, CommonAttributeIds::Range, 300.f
		));
		const sf::Vector2f origin = context.owner.GetActorLocation();
		sf::Vector2f target = world->GetMouseWorldPosition();
		sf::Vector2f offset = target - origin;
		const float distance = GetVectorLength(offset);
		if (distance > castRange)
		{
			target = origin + offset / distance * castRange;
			offset = target - origin;
		}
		sf::Vector2f direction = offset;
		if (GetVectorLength(direction) <= Epsilon)
		{
			direction = context.owner.GetActorForwardDirection();
		}
		if (GetVectorLength(direction) <= Epsilon)
		{
			direction = { 0.f, -1.f };
		}
		else
		{
			NormalizeVector(direction);
		}

		AbilityExecutionContext executionContext{
			&context.abilitySystem, &context.definition, nullptr, &context.instance
		};
		const shared_ptr<AbilityWorldActor> spawned =
			AbilityActorSpawner::SpawnAtLocation(
				AbilityData::FoldspaceArena::Actor::Arena::BasicDefinitionId,
				executionContext,
				context.owner,
				origin,
				direction
			).lock();
		const auto arena = std::dynamic_pointer_cast<FoldspaceArenaActor>(spawned);
		if (!arena)
		{
			return false;
		}

		arena->ConfigureFromAbilityValues(
			values,
			std::max(0.f, context.abilitySystem.GetAttributes().GetCurrentValue(
				OwnerAttributeIds::EnergyMax
			))
		);
		arena->SetSnapshotTarget(target);
		mArena = arena;
		mElapsed = 0.f;
		mMinimumArenaDuration = std::max(0.f, sas::FindAttributeValue(
			values,
			AbilityData::FoldspaceArena::Attribute::MinimumArenaDuration,
			1.f
		));
		mCancelAvailable = false;
		mActive = true;
		context.abilitySystem.AddOwnedTag(AbilityData::FoldspaceArena::State::Active);
		EmitEvent(context, AbilityData::FoldspaceArena::Event::Started);
		return true;
	}

	bool FoldspaceArenaAbility::OnInputPressed(GameAbilityBehaviorContext& context)
	{
		if (!mActive || !mCancelAvailable)
		{
			return mActive;
		}
		context.instance.Cancel(sas::AbilityEndReason::Cancelled);
		return true;
	}

	void FoldspaceArenaAbility::Tick(
		GameAbilityBehaviorContext& context,
		float deltaTime
	)
	{
		if (!mActive)
		{
			return;
		}
		mElapsed += std::max(0.f, deltaTime);
		if (!mCancelAvailable && mElapsed >= mMinimumArenaDuration)
		{
			mCancelAvailable = true;
			context.abilitySystem.AddOwnedTag(
				AbilityData::FoldspaceArena::State::CancelAvailable
			);
			EmitEvent(context, AbilityData::FoldspaceArena::Event::CancelAvailable);
		}
		if (const shared_ptr<FoldspaceArenaActor> arena = mArena.lock();
			!arena || arena->GetIsPendingDestroy())
		{
			context.instance.Cancel(sas::AbilityEndReason::DurationExpired);
		}
	}

	void FoldspaceArenaAbility::End(
		GameAbilityBehaviorContext& context,
		sas::AbilityEndReason reason
	)
	{
		(void)reason;
		ClearRuntimeState(context);
		EmitEvent(context, AbilityData::FoldspaceArena::Event::Ended);
	}

	void FoldspaceArenaAbility::ClearRuntimeState(GameAbilityBehaviorContext& context)
	{
		if (!mActive)
		{
			return;
		}
		if (const shared_ptr<FoldspaceArenaActor> arena = mArena.lock())
		{
			arena->Destroy();
		}
		mArena.reset();
		context.abilitySystem.RemoveOwnedTag(AbilityData::FoldspaceArena::State::Active);
		context.abilitySystem.RemoveOwnedTag(
			AbilityData::FoldspaceArena::State::CancelAvailable
		);
		mElapsed = 0.f;
		mCancelAvailable = false;
		mActive = false;
	}

	void FoldspaceArenaAbility::EmitEvent(
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
