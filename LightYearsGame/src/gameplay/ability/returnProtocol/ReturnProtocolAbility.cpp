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
			!std::isfinite(definition.cooldown) || definition.cooldown <= 0.f)
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
			AbilityData::ReturnProtocol::Attribute::MaxHealthReference,
			AbilityData::ReturnProtocol::Attribute::MaxHealthDamageScale
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
		return true;
	}

	bool ReturnProtocolAbility::Activate(GameAbilityBehaviorContext& context)
	{
		if (mActive || !ProjectileReflectionService::RegisterReceiver(
			context.owner,
			*this
		))
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
		if (!mActive)
		{
			return;
		}

		ProjectileReflectionService::UnregisterReceiver(context.owner, *this);
		context.abilitySystem.RemoveOwnedTag(AbilityData::ReturnProtocol::State::Active);
		if (const shared_ptr<ReturnProtocolVisualActor> visual = mVisualActor.lock())
		{
			visual->Destroy();
		}
		mVisualActor.reset();
		mOwner = nullptr;
		mReflectDamageMultiplier = 1.f;
		mActive = false;
		EmitEvent(context, AbilityData::ReturnProtocol::Event::Ended);
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
		const float reference = std::max(0.f, sas::FindAttributeValue(
			values,
			AbilityData::ReturnProtocol::Attribute::MaxHealthReference,
			100.f
		));
		const float maxHealthScale = std::max(0.f, sas::FindAttributeValue(
			values,
			AbilityData::ReturnProtocol::Attribute::MaxHealthDamageScale,
			0.001f
		));
		const float maxHealth = std::max(0.f, context.abilitySystem.GetAttributes()
			.GetCurrentValue(OwnerAttributeIds::MaxHealth));
		return baseMultiplier + std::max(0.f, maxHealth - reference) * maxHealthScale;
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
