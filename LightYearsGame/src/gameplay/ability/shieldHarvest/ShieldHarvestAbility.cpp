#include "gameplay/ability/shieldHarvest/ShieldHarvestAbility.h"

#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/actors/AreaTelegraphActor.h"
#include "gameplay/ability/shieldHarvest/ShieldHarvestContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/portal/PortalTransferParticipant.h"
#include "gameplay/targeting/CombatantTargetQuery.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/shieldHarvest/ShieldHarvestPresentationIds.h"
#include "presentation/ability/shieldHarvest/ShieldHarvestPresentationProfile.h"
#include "spaceShip/SpaceShip.h"
#include "framework/World.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		float FindValue(
			const sas::GameplayAttributeList& values,
			const sas::AttributeId& attributeId,
			float fallback
		)
		{
			return sas::FindAttributeValue(values, attributeId, fallback);
		}

		sas::GameplayAttributeList ResolveValues(
			GameAbilityBehaviorContext& context
		)
		{
			AbilityExecutionContext executionContext{
				&context.abilitySystem,
				&context.definition,
				nullptr,
				&context.instance
			};
			return AbilityActionAttributeResolver::ResolveAbilityAttributes(executionContext);
		}

		bool IsFiniteNonNegative(float value)
		{
			return std::isfinite(value) && value >= 0.f;
		}
	}

	bool ShieldHarvestAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		if (definition.abilityId != AbilityData::ShieldHarvest::AbilityId::Basic ||
			!sas::IsLoadoutAbilitySlot(definition.slot) ||
			definition.activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
			definition.lifetimePolicy != sas::AbilityLifetimePolicy::Duration ||
			definition.maxCharges != 1 ||
			!std::isfinite(definition.cooldown) || definition.cooldown <= 0.f ||
			!std::isfinite(definition.duration) || definition.duration <= 0.f)
		{
			if (failureReason)
			{
				*failureReason =
					"Shield Harvest requires a loadout slot, pressed activation, and one charge.";
			}
			return false;
		}

		for (const sas::AttributeId& required : {
			AbilityData::ShieldHarvest::Attribute::Radius,
			AbilityData::ShieldHarvest::Attribute::ShieldPerEnemy,
			AbilityData::ShieldHarvest::Attribute::OvershieldHoldDuration,
			AbilityData::ShieldHarvest::Attribute::OvershieldDecayPerSecond
		})
		{
			const sas::GameplayAttribute* attribute = sas::FindAttribute(
				definition.attributes,
				required
			);
			if (!attribute || !std::isfinite(attribute->baseValue))
			{
				if (failureReason)
				{
					*failureReason = "Shield Harvest must declare all runtime attributes.";
				}
				return false;
			}
		}

		const sas::GameplayAttribute* radius = sas::FindAttribute(
			definition.attributes,
			AbilityData::ShieldHarvest::Attribute::Radius
		);
		const sas::GameplayAttribute* shieldPerEnemy = sas::FindAttribute(
			definition.attributes,
			AbilityData::ShieldHarvest::Attribute::ShieldPerEnemy
		);
		const sas::GameplayAttribute* holdDuration = sas::FindAttribute(
			definition.attributes,
			AbilityData::ShieldHarvest::Attribute::OvershieldHoldDuration
		);
		const sas::GameplayAttribute* decayPerSecond = sas::FindAttribute(
			definition.attributes,
			AbilityData::ShieldHarvest::Attribute::OvershieldDecayPerSecond
		);
		if (radius->baseValue <= 0.f || shieldPerEnemy->baseValue <= 0.f ||
			!IsFiniteNonNegative(holdDuration->baseValue) ||
			!IsFiniteNonNegative(decayPerSecond->baseValue))
		{
			if (failureReason)
			{
				*failureReason = "Shield Harvest contains an invalid radius or overshield value.";
			}
			return false;
		}

		if (definition.levelProgression.size() != 14)
		{
			if (failureReason)
			{
				*failureReason = "Shield Harvest requires fourteen level progression steps.";
			}
			return false;
		}
		return true;
	}

	bool ShieldHarvestAbility::Activate(GameAbilityBehaviorContext& context)
	{
		if (!dynamic_cast<SpaceShip*>(&context.owner) || mHarvested)
		{
			return false;
		}

		const sas::GameplayAttributeList values = ResolveValues(context);
		const float radius = std::max(
			1.f,
			FindValue(values, AbilityData::ShieldHarvest::Attribute::Radius, 700.f)
		);
		const float focusDuration = std::max(0.001f, context.definition.duration);
		mFocusElapsed = 0.f;
		mHarvested = false;
		context.abilitySystem.AddOwnedTag(AbilityData::ShieldHarvest::State::Focusing);

		if (World* world = context.owner.GetWorld())
		{
			if (const ShieldHarvestPresentationProfile* profile =
				PresentationProfileRegistry<ShieldHarvestPresentationProfile>::Find(
					ShieldHarvestPresentationIds::FocusBasic
				))
			{
				mTelegraph = world->SpawnActor<AreaTelegraphActor>(
					AreaTelegraphActor::SpawnParams{
						context.owner.GetActorLocation(),
						radius,
						0.f,
						profile->focusTelegraph,
						AreaTelegraphAnchorMode::FollowActor,
						AreaTelegraphProgressDriver::External,
						&context.owner
					}
				);
			}
		}

		EmitEvent(context, AbilityData::ShieldHarvest::Event::Started);
		return true;
	}

	void ShieldHarvestAbility::Tick(
		GameAbilityBehaviorContext& context,
		float deltaTime
	)
	{
		if (mHarvested)
		{
			return;
		}
		if (const auto* participant = dynamic_cast<const PortalTransferParticipant*>(
			&context.owner); participant && participant->IsInPortalTransit())
		{
			// The enemy count is a single completion-edge snapshot. Pause the
			// focus rather than resolving it from the hidden entrance position.
			return;
		}

		mFocusElapsed += std::max(0.f, deltaTime);
		const float focusDuration = std::max(0.001f, context.definition.duration);
		const float progress = std::clamp(mFocusElapsed / focusDuration, 0.f, 1.f);
		if (const shared_ptr<AreaTelegraphActor> telegraph = mTelegraph.lock())
		{
			telegraph->SetExternalProgress(progress);
		}

		if (progress < 1.f)
		{
			return;
		}

		SpaceShip* ship = dynamic_cast<SpaceShip*>(&context.owner);
		World* world = context.owner.GetWorld();
		if (!ship || !world)
		{
			return;
		}

		// The query is intentionally executed once at the completion edge. Enemies
		// entering or leaving the area during overshield hold do not change the grant.
		const sas::GameplayAttributeList values = ResolveValues(context);
		const float radius = std::max(
			1.f,
			FindValue(values, AbilityData::ShieldHarvest::Attribute::Radius, 700.f)
		);
		const float shieldPerEnemy = std::max(
			0.f,
			FindValue(values, AbilityData::ShieldHarvest::Attribute::ShieldPerEnemy, 40.f)
		);
		const float holdDuration = std::max(
			0.f,
			FindValue(values, AbilityData::ShieldHarvest::Attribute::OvershieldHoldDuration, 5.f)
		);
		const float decayPerSecond = std::max(
			0.f,
			FindValue(values, AbilityData::ShieldHarvest::Attribute::OvershieldDecayPerSecond, 100.f)
		);
		const std::size_t enemyCount = targeting::FindOpposingCombatants(
			*world,
			context.owner,
			radius
		).size();
		ship->GetShieldComponent().GrantTemporaryOvershield(
			context.definition.abilityId,
			static_cast<float>(enemyCount) * shieldPerEnemy,
			holdDuration,
			decayPerSecond
		);
		if (const shared_ptr<AreaTelegraphActor> telegraph = mTelegraph.lock())
		{
			// Keep the completed area visible briefly so the player receives
			// explicit feedback that the enemy count was captured.
			telegraph->Complete();
		}
		mHarvested = true;
		EmitEvent(context, AbilityData::ShieldHarvest::Event::Harvested);
	}

	void ShieldHarvestAbility::End(
		GameAbilityBehaviorContext& context,
		sas::AbilityEndReason reason
	)
	{
		(void)reason;
		context.abilitySystem.RemoveOwnedTag(AbilityData::ShieldHarvest::State::Focusing);
		if (const shared_ptr<AreaTelegraphActor> telegraph = mTelegraph.lock())
		{
			if (!telegraph->IsInCompletionFeedback())
			{
				telegraph->Destroy();
			}
		}
		mTelegraph.reset();
		mFocusElapsed = 0.f;
		mHarvested = false;
		EmitEvent(context, AbilityData::ShieldHarvest::Event::Ended);
	}

	void ShieldHarvestAbility::EmitEvent(
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
