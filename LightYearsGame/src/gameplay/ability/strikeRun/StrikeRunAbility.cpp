#include "gameplay/ability/strikeRun/StrikeRunAbility.h"

#include "attributes/AttributeSystem.h"
#include "framework/World.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/actors/AbilityActorSpawner.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/ability/strikeRun/StrikeRunBombardmentActor.h"
#include "gameplay/ability/strikeRun/StrikeRunContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/ability/AbilityActorStructs.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		constexpr std::size_t RequiredProgressionStepCount = 14;
		constexpr float BaseCooldown = 16.f;
		constexpr float BaseDuration = 1.4f;
		constexpr float AttackPowerScale = 0.45f;
		constexpr float DamagePerLevel = 4.f;
		constexpr float CooldownPerLevel = -0.25f;
		constexpr float Epsilon = 0.0001f;

		bool NearlyEqual(float left, float right)
		{
			return std::isfinite(left) && std::isfinite(right) &&
				std::abs(left - right) <= Epsilon;
		}

		bool HasExactAbilityTags(const GameAbilityDefinition& definition)
		{
			if (definition.abilityTags.size() != 2)
			{
				return false;
			}

			bool hasCategory = false;
			bool hasFamily = false;
			for (const GameplayTag& tag : definition.abilityTags)
			{
				hasCategory = hasCategory || tag.MatchesTagExact(
					AbilityData::StrikeRun::CategoryTag
				);
				hasFamily = hasFamily || tag.MatchesTagExact(
					AbilityData::StrikeRun::FamilyTag
				);
			}
			return hasCategory && hasFamily;
		}

		bool HasExpectedModifier(
			const AbilityLevelStep& step,
			const sas::AttributeId& attributeId,
			float magnitude
		)
		{
			return std::any_of(
				step.attributeModifiers.begin(),
				step.attributeModifiers.end(),
				[&](const sas::AttributeModifier& modifier)
				{
					return modifier.attributeId == attributeId &&
						modifier.operation == sas::AttributeModifierOperation::Add &&
						NearlyEqual(modifier.magnitude, magnitude);
				}
			);
		}

		sas::GameplayAttributeList ResolveBombardmentValues(
			GameAbilityBehaviorContext& context
		)
		{
			const AbilityActorDefinition* actorDefinition =
				AbilityData::FindAbilityActorDefinition(
					AbilityData::StrikeRun::Actor::Bombardment::BasicDefinitionId
				);
			if (!actorDefinition)
			{
				return {};
			}

			AbilityExecutionContext executionContext{
				&context.abilitySystem,
				&context.definition,
				nullptr,
				&context.instance
			};
		const List<GameplayTag> damageTags =
				AbilityActionAttributeResolver::ResolveDamageTags(
					executionContext,
					AttachmentHostKind::Ability
				);
			return AbilityActionAttributeResolver::ResolveAttributes(
				executionContext,
				nullptr,
				actorDefinition->attributes,
				damageTags
			);
		}

		float FindValue(
			const sas::GameplayAttributeList& values,
			const sas::AttributeId& id,
			float fallback
		)
		{
			return sas::FindAttributeValue(values, id, fallback);
		}

		sf::Vector2f NormalizeOrDefault(
			const sf::Vector2f& value,
			const sf::Vector2f& fallback
		)
		{
			const float length = GetVectorLength(value);
			return std::isfinite(length) && length > 0.001f
				? value / length
				: fallback;
		}
	}

	bool StrikeRunAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		const bool validIdentity =
			definition.abilityId == AbilityData::StrikeRun::AbilityId::Basic &&
			definition.behaviorType == AbilityBehaviorType::StrikeRun &&
			HasExactAbilityTags(definition);
		const bool validLifecycle =
			sas::IsLoadoutAbilitySlot(definition.slot) &&
			definition.activationPolicy == sas::AbilityActivationPolicy::OnPressed &&
			definition.lifetimePolicy == sas::AbilityLifetimePolicy::Duration &&
			definition.maxCharges == 1 &&
			NearlyEqual(definition.cooldown, BaseCooldown) &&
			NearlyEqual(definition.duration, BaseDuration);
		const bool validDamage = definition.damageTags.size() == 1 &&
			definition.damageTags.front().MatchesTagExact(DamageTypeSchema::Kinetic);
		const AbilityActorDefinition* actorDefinition =
			AbilityData::FindAbilityActorDefinition(
				AbilityData::StrikeRun::Actor::Bombardment::BasicDefinitionId
			);
		bool validActor = actorDefinition != nullptr;
		if (validActor)
		{
			for (const sas::AttributeId& required : {
				CommonAttributeIds::Damage,
				CommonAttributeIds::Radius,
				CommonAttributeIds::Range,
				AbilityData::StrikeRun::Attribute::ImpactCount,
				AbilityData::StrikeRun::Attribute::ImpactSpan,
				AbilityData::StrikeRun::Attribute::TargetingWindow,
				AbilityData::StrikeRun::Attribute::FinalTelegraphDuration,
				AbilityData::StrikeRun::Attribute::ImpactDelay
			})
			{
				const sas::GameplayAttribute* attribute = sas::FindAttribute(
					actorDefinition->attributes,
					required
				);
				if (!attribute || !std::isfinite(attribute->baseValue))
				{
					validActor = false;
					break;
				}
			}
		}

		bool validProgression = definition.levelProgression.size() ==
			RequiredProgressionStepCount;
		if (validProgression)
		{
			float resolvedCooldown = definition.cooldown;
			for (const AbilityLevelStep& step : definition.levelProgression)
			{
				if (step.attributeModifiers.size() != 2 ||
					!HasExpectedModifier(step, CommonAttributeIds::Damage, DamagePerLevel) ||
					!HasExpectedModifier(step, CommonAttributeIds::Cooldown, CooldownPerLevel))
				{
					validProgression = false;
					break;
				}
				resolvedCooldown += CooldownPerLevel;
				if (resolvedCooldown <= 0.f)
				{
					validProgression = false;
					break;
				}
			}
		}

		bool validScaling = definition.scalingRules.size() == 1;
		if (validScaling)
		{
			const sas::AttributeScalingRule& rule = definition.scalingRules.front();
			validScaling = rule.targetAttributeId == CommonAttributeIds::Damage &&
				rule.sourceAttributeId == OwnerAttributeIds::AttackPower &&
				rule.operation == sas::AttributeModifierOperation::Add &&
				NearlyEqual(rule.coefficient, AttackPowerScale);
		}

		if (!validIdentity || !validLifecycle || !validDamage || !validActor ||
			!validProgression || !validScaling || !definition.actions.empty())
		{
			if (failureReason)
			{
				*failureReason =
					"Strike Run requires a five-impact Kinetic bombardment, two-stage targeting, and AttackPower scaling.";
			}
			return false;
		}
		return true;
	}

	bool StrikeRunAbility::Activate(GameAbilityBehaviorContext& context)
	{
		World* world = context.owner.GetWorld();
		if (!world || mWaitingForDirection || mBombardmentActor.lock())
		{
			return false;
		}

		const sas::GameplayAttributeList values = ResolveBombardmentValues(context);
		const float castRange = std::max(
			0.f,
			FindValue(
				values,
				AbilityData::StrikeRun::Attribute::CenterRange,
				1200.f
			)
		);
		if (castRange <= 0.f)
		{
			return false;
		}

		const sf::Vector2f ownerLocation = context.owner.GetActorLocation();
		const sf::Vector2f cursorLocation = world->GetMouseWorldPosition();
		const sf::Vector2f toCursor = cursorLocation - ownerLocation;
		const float cursorDistance = GetVectorLength(toCursor);
		const sf::Vector2f anchorDirection = NormalizeOrDefault(
			toCursor,
			{ 1.f, 0.f }
		);
		const sf::Vector2f anchorLocation = ownerLocation + anchorDirection *
			std::min(castRange, std::max(0.f, cursorDistance));
		const sf::Vector2f previewDirection = NormalizeOrDefault(
			cursorLocation - anchorLocation,
			anchorDirection
		);

		AbilityExecutionContext executionContext{
			&context.abilitySystem,
			&context.definition,
			nullptr,
			&context.instance
		};
		const weak_ptr<AbilityWorldActor> spawned = AbilityActorSpawner::SpawnAtLocation(
			AbilityData::StrikeRun::Actor::Bombardment::BasicDefinitionId,
			executionContext,
			context.owner,
			anchorLocation,
			previewDirection
		);
		const shared_ptr<StrikeRunBombardmentActor> actor =
			std::dynamic_pointer_cast<StrikeRunBombardmentActor>(spawned.lock());
		if (!actor)
		{
			return false;
		}

		actor->SetPreviewDirection(previewDirection);
		mBombardmentActor = actor;
		mTargetingElapsed = 0.f;
		mTargetingWindow = std::max(
			0.01f,
			FindValue(
				values,
				AbilityData::StrikeRun::Attribute::TargetingWindow,
				3.f
			)
		);
		mWaitingForDirection = true;
		mConfirmed = false;
		context.instance.DeferActiveDurationStart();
		context.abilitySystem.AddOwnedTag(AbilityData::StrikeRun::State::Targeting);
		EmitEvent(context, AbilityData::StrikeRun::Event::Started);
		return true;
	}

	bool StrikeRunAbility::OnInputPressed(GameAbilityBehaviorContext& context)
	{
		if (!mWaitingForDirection)
		{
			return false;
		}

		const shared_ptr<StrikeRunBombardmentActor> actor = mBombardmentActor.lock();
		if (!actor || !context.owner.GetWorld())
		{
			return true;
		}

		const sf::Vector2f direction = NormalizeOrDefault(
			context.owner.GetWorld()->GetMouseWorldPosition() -
				actor->GetActorLocation(),
			actor->GetDirection()
		);
		actor->SetPreviewDirection(direction);
		actor->Confirm();
		mWaitingForDirection = false;
		mConfirmed = true;
		context.abilitySystem.RemoveOwnedTag(AbilityData::StrikeRun::State::Targeting);
		context.abilitySystem.AddOwnedTag(AbilityData::StrikeRun::State::Confirmed);
		EmitEvent(context, AbilityData::StrikeRun::Event::DirectionLocked);
		// Start the normal active clock now that targeting is confirmed. Passing
		// zero would leave the duration at zero, and the shared runtime treats that
		// as an indefinitely active ability instead of ending it.
		context.instance.StartDeferredActiveDuration(
			context.instance.GetActiveDuration()
		);
		return true;
	}

	void StrikeRunAbility::Tick(
		GameAbilityBehaviorContext& context,
		float deltaTime
	)
	{
		if (!mWaitingForDirection)
		{
			return;
		}

		UpdatePreviewDirection(context);
		mTargetingElapsed += std::max(0.f, deltaTime);
		if (mTargetingElapsed >= mTargetingWindow)
		{
			context.instance.Cancel(sas::AbilityEndReason::Cancelled);
		}
	}

	void StrikeRunAbility::End(
		GameAbilityBehaviorContext& context,
		sas::AbilityEndReason reason
	)
	{
		if (!mConfirmed)
		{
			if (const shared_ptr<StrikeRunBombardmentActor> actor = mBombardmentActor.lock())
			{
				actor->CancelPreview();
			}
		}
		ClearTargetingState(context);
		mBombardmentActor.reset();
		mWaitingForDirection = false;
		mTargetingElapsed = 0.f;
		(void)reason;
		EmitEvent(context, AbilityData::StrikeRun::Event::Ended);
	}

	float StrikeRunAbility::ResolveCooldownDurationOnEnd(
		GameAbilityBehaviorContext& context,
		sas::AbilityEndReason reason,
		float resolvedCooldown
	)
	{
		(void)context;
		// A timeout/cancel before the second press is only a targeting preview and
		// must not consume the ability cooldown.
		if (reason == sas::AbilityEndReason::Cancelled && !mConfirmed)
		{
			return 0.f;
		}
		return resolvedCooldown;
	}

	void StrikeRunAbility::EmitEvent(
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

	void StrikeRunAbility::UpdatePreviewDirection(
		GameAbilityBehaviorContext& context
	) const
	{
		const shared_ptr<StrikeRunBombardmentActor> actor = mBombardmentActor.lock();
		if (!actor || !context.owner.GetWorld())
		{
			return;
		}
		actor->SetPreviewDirection(
			NormalizeOrDefault(
				context.owner.GetWorld()->GetMouseWorldPosition() -
					actor->GetActorLocation(),
				actor->GetDirection()
			)
		);
	}

	void StrikeRunAbility::ClearTargetingState(
		GameAbilityBehaviorContext& context
	) const
	{
		context.abilitySystem.RemoveOwnedTag(AbilityData::StrikeRun::State::Targeting);
		context.abilitySystem.RemoveOwnedTag(AbilityData::StrikeRun::State::Confirmed);
	}
}
