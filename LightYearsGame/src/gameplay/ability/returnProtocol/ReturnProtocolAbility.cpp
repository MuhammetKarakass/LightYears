#include "gameplay/ability/returnProtocol/ReturnProtocolAbility.h"

#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/ability/returnProtocol/ReturnProtocolContracts.h"
#include "gameplay/ability/returnProtocol/ReturnProtocolVisualActor.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/returnProtocol/ReturnProtocolPresentationIds.h"
#include "presentation/ability/returnProtocol/ReturnProtocolPresentationProfile.h"
#include "framework/MathUtility.h"
#include "framework/World.h"

#include <algorithm>
#include <cmath>
#include <exception>

namespace ly
{
	namespace
	{
		bool IsFiniteNonNegative(float value)
		{
			return std::isfinite(value) && value >= 0.f;
		}

		sf::Vector2f ResolveReturnDirection(
			const AbilityWorldActor& projectile,
			const Actor* originalOwner
		)
		{
			sf::Vector2f direction = originalOwner
				? originalOwner->GetActorLocation() - projectile.GetActorLocation()
				: -projectile.GetVelocity();
			if (GetVectorLength(direction) <= 0.001f)
			{
				direction = -projectile.GetActorForwardDirection();
			}
			return direction;
		}
	}

	bool ReturnProtocolAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		if (definition.abilityId != AbilityData::ReturnProtocol::AbilityId::Basic ||
			!sas::IsLoadoutAbilitySlot(definition.slot) ||
			definition.activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
			definition.lifetimePolicy != sas::AbilityLifetimePolicy::Duration ||
			definition.maxCharges != 1 || definition.duration <= 0.f ||
			!std::isfinite(definition.cooldown) || definition.cooldown <= 0.f ||
			definition.attributes.size() != 5 || definition.levelProgression.size() != 14)
		{
			if (failureReason)
			{
				*failureReason =
					"Return Protocol requires OnPressed + Duration, one charge, and positive cooldown/duration.";
			}
			return false;
		}

		for (const sas::AttributeId& attributeId : {
			AbilityData::ReturnProtocol::Attribute::BaseReflectDamageMultiplier,
			AbilityData::ReturnProtocol::Attribute::AttackPowerReference,
			AbilityData::ReturnProtocol::Attribute::AttackPowerScale,
			AbilityData::ReturnProtocol::Attribute::EnergyPowerReference,
			AbilityData::ReturnProtocol::Attribute::EnergyPowerScale
		})
		{
			const sas::GameplayAttribute* attribute = sas::FindAttribute(
				definition.attributes,
				attributeId
			);
			if (!attribute || !IsFiniteNonNegative(attribute->baseValue))
			{
				if (failureReason)
				{
					*failureReason = "Return Protocol must declare valid reflection attributes.";
				}
				return false;
			}
		}
		for (std::size_t stepIndex = 0; stepIndex < definition.levelProgression.size(); ++stepIndex)
		{
			const AbilityLevelStep& step = definition.levelProgression[stepIndex];
			bool baseScaleMatches = false;
			bool attackScaleMatches = false;
			bool energyScaleMatches = false;
			bool cooldownMatches = false;
			for (const sas::AttributeModifier& modifier : step.attributeModifiers)
			{
				if (modifier.operation != sas::AttributeModifierOperation::Add || !std::isfinite(modifier.magnitude)) continue;
				if (modifier.attributeId == AbilityData::ReturnProtocol::Attribute::BaseReflectDamageMultiplier)
				{
					baseScaleMatches = std::abs(modifier.magnitude - 0.04f) <= 0.0001f;
				}
				else if (modifier.attributeId == AbilityData::ReturnProtocol::Attribute::AttackPowerScale)
				{
					attackScaleMatches = std::abs(modifier.magnitude - 0.01f) <= 0.0001f;
				}
				else if (modifier.attributeId == AbilityData::ReturnProtocol::Attribute::EnergyPowerScale)
				{
					energyScaleMatches = std::abs(modifier.magnitude - 0.01f) <= 0.0001f;
				}
				else if (modifier.attributeId == CommonAttributeIds::Cooldown)
				{
					const float expected = -GetGlobalAbilityCooldownStepReduction(definition.cooldown, stepIndex);
					cooldownMatches = std::abs(modifier.magnitude - expected) <= 0.0001f;
				}
			}
			if (step.attributeModifiers.size() != 4 || !baseScaleMatches || !attackScaleMatches ||
				!energyScaleMatches || !cooldownMatches || !step.unlockedUpgradeIds.empty() ||
				!step.addedActions.empty() || !step.addedTriggers.empty() || !step.scalingRules.empty())
			{
				if (failureReason) *failureReason = "Return Protocol progression must add its three damage scalars and global cooldown at every level.";
				return false;
			}
		}
		return true;
	}

	bool ReturnProtocolAbility::Activate(GameAbilityBehaviorContext& context)
	{
		if (mActive || mVisualDestroyPending)
		{
			return false;
		}
		mEndTagRemovalAttempted = false;
		mEndedEventAttempted = false;

		mRegistration = ProjectileReflectionService::RegisterReceiver(context.owner, *this);
		if (!mRegistration.IsValid())
		{
			return false;
		}

		mOwner = &context.owner;
		mReflectDamageMultiplier = ResolveReflectDamageMultiplier(context);
		mActive = true;
		context.abilitySystem.AddOwnedTag(AbilityData::ReturnProtocol::State::Active);

		if (World* world = context.owner.GetWorld())
		{
			if (const ReturnProtocolPresentationProfile* profile =
				PresentationProfileRegistry<ReturnProtocolPresentationProfile>::Find(
					ReturnProtocolPresentationIds::Basic
				))
			{
				mVisualActor = world->SpawnActor<ReturnProtocolVisualActor>(
					&context.owner,
					*profile
				);
			}
		}

		EmitEvent(context, AbilityData::ReturnProtocol::Event::Started);
		return true;
	}

	void ReturnProtocolAbility::End(
		GameAbilityBehaviorContext& context,
		sas::AbilityEndReason reason
	)
	{
		(void)reason;
		if (mEnding || (!mActive && !mVisualDestroyPending))
		{
			return;
		}
		mEnding = true;
		struct EndScope
		{
			bool& ending;
			~EndScope() { ending = false; }
		} endScope{ mEnding };

		const bool wasActive = mActive;
		std::exception_ptr error;
		const auto cleanup = [&error](auto&& operation)
		{
			try { operation(); }
			catch (...) { if (!error) error = std::current_exception(); }
		};
		if (wasActive)
		{
			// The token owns the registration: explicit reset handles normal End(),
			// while its destructor covers paths that never reach End().
			cleanup([&] { mRegistration.Reset(); });
			mOwner = nullptr;
			mReflectDamageMultiplier = 1.f;
			mActive = false;
			mVisualDestroyPending = !mVisualActor.expired();
		}

		if (mVisualDestroyPending)
		{
			const auto visual = mVisualActor.lock();
			if (!visual)
			{
				mVisualActor.reset();
				mVisualDestroyPending = false;
			}
			else
			{
				cleanup([&] { visual->Destroy(); });
				if (visual->GetIsPendingDestroy())
				{
					mVisualActor.reset();
					mVisualDestroyPending = false;
				}
			}
		}

		if (wasActive && !mEndTagRemovalAttempted)
		{
			mEndTagRemovalAttempted = true;
			cleanup([&] { context.abilitySystem.RemoveOwnedTag(AbilityData::ReturnProtocol::State::Active); });
		}
		if (wasActive && !mEndedEventAttempted)
		{
			mEndedEventAttempted = true;
			cleanup([&] { EmitEvent(context, AbilityData::ReturnProtocol::Event::Ended); });
		}
		if (error) std::rethrow_exception(error);
	}

	bool ReturnProtocolAbility::TryReflectIncomingProjectile(
		AbilityWorldActor& projectile,
		Actor& defender
	)
	{
		if (!mActive || mOwner != &defender || !projectile.CanBeReflected())
		{
			return false;
		}

		Actor* originalOwner = projectile.GetOwnerActor();
		if (!originalOwner || originalOwner == &defender)
		{
			return false;
		}

		const ProjectileReflectionRequest request{
			defender,
			ResolveReturnDirection(projectile, originalOwner),
			mReflectDamageMultiplier
		};
		const bool reflected = projectile.TryReflectProjectile(request);
		if (reflected)
		{
			EmitReflectionEvent(defender, projectile);
		}
		return reflected;
	}

	float ReturnProtocolAbility::ResolveReflectDamageMultiplier(
		GameAbilityBehaviorContext& context
	) const
	{
		AbilityExecutionContext executionContext{
			&context.abilitySystem,
			&context.definition,
			nullptr,
			&context.instance
		};
		const sas::GameplayAttributeList values =
			AbilityActionAttributeResolver::ResolveAbilityAttributes(executionContext);
		const float baseMultiplier = std::max(0.f, sas::FindAttributeValue(
			values,
			AbilityData::ReturnProtocol::Attribute::BaseReflectDamageMultiplier,
			0.80f
		));
		const float attackPowerReference = std::max(1.f, sas::FindAttributeValue(
			values,
			AbilityData::ReturnProtocol::Attribute::AttackPowerReference,
			100.f
		));
		const float attackPowerScale = std::max(0.f, sas::FindAttributeValue(
			values,
			AbilityData::ReturnProtocol::Attribute::AttackPowerScale,
			0.08f
		));
		const float energyPowerReference = std::max(1.f, sas::FindAttributeValue(
			values,
			AbilityData::ReturnProtocol::Attribute::EnergyPowerReference,
			100.f
		));
		const float energyPowerScale = std::max(0.f, sas::FindAttributeValue(
			values,
			AbilityData::ReturnProtocol::Attribute::EnergyPowerScale,
			0.12f
		));
		const sas::AttributeSystem& ownerAttributes = context.abilitySystem.GetAttributes();
		const float attackPower = std::max(0.f, ownerAttributes.GetCurrentValue(OwnerAttributeIds::AttackPower));
		const float energyPower = std::max(0.f, ownerAttributes.GetCurrentValue(OwnerAttributeIds::EnergyPower));
		return baseMultiplier + (attackPower / attackPowerReference) * attackPowerScale +
			(energyPower / energyPowerReference) * energyPowerScale;
	}

	void ReturnProtocolAbility::EmitEvent(
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

	void ReturnProtocolAbility::EmitReflectionEvent(
		Actor& defender,
		AbilityWorldActor& projectile
	) const
	{
		auto* combatant = dynamic_cast<Combatant*>(&defender);
		if (!combatant)
		{
			return;
		}

		sas::AbilityEvent event;
		event.eventTag = AbilityData::ReturnProtocol::Event::Reflected;
		event.sourceAbilityId = sas::ContentId{ AbilityData::ReturnProtocol::AbilityId::Basic };
		event.sourceAbilityTags = {
			AbilityData::ReturnProtocol::CategoryTag,
			AbilityData::ReturnProtocol::FamilyTag
		};
		event.SetSource(&defender);
		event.SetTarget(&projectile);
		combatant->GetAbilitySystemComponent().HandleGameplayEvent(event);
	}
}
