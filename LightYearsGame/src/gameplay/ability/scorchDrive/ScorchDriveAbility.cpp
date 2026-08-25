#include "gameplay/ability/scorchDrive/ScorchDriveAbility.h"

#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/actors/AbilityActorSpawner.h"
#include "gameplay/ability/scorchDrive/ScorchDriveContracts.h"
#include "gameplay/ability/scorchDrive/ScorchDriveFireSegmentActor.h"
#include "gameplay/ability/scorchDrive/ScorchDriveTrailCoordinator.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/portal/PortalTransferParticipant.h"
#include "gameplay/tags/GameplayTags.h"
#include "player/PlayerSpaceShip.h"
#include "spaceShip/SpaceShip.h"
#include "framework/World.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		sas::GameplayAttributeList ResolveValues(GameAbilityBehaviorContext& context)
		{
			AbilityExecutionContext executionContext{
				&context.abilitySystem,
				&context.definition,
				nullptr,
				&context.instance
			};
			return AbilityActionAttributeResolver::ResolveAbilityAttributes(
				executionContext
			);
		}

		float FindValue(
			const sas::GameplayAttributeList& values,
			const sas::AttributeId& attributeId,
			float fallback
		)
		{
			return sas::FindAttributeValue(values, attributeId, fallback);
		}

		bool IsFinitePositive(float value)
		{
			return std::isfinite(value) && value > 0.f;
		}

		sf::Vector2f NormalizeDirection(const sf::Vector2f& value)
		{
			if (GetVectorLength(value) <= 0.001f)
			{
				return { 0.f, -1.f };
			}
			sf::Vector2f direction = value;
			NormalizeVector(direction);
			return direction;
		}
	}

	bool ScorchDriveAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		if (definition.abilityId != AbilityData::ScorchDrive::AbilityId::Basic ||
			!sas::IsLoadoutAbilitySlot(definition.slot) ||
			definition.activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
			definition.lifetimePolicy != sas::AbilityLifetimePolicy::Duration ||
			definition.maxCharges != 1 ||
			!IsFinitePositive(definition.cooldown) ||
			!IsFinitePositive(definition.duration))
		{
			if (failureReason)
			{
				*failureReason =
					"Scorch Drive requires a pressed, duration-based, one-charge loadout ability.";
			}
			return false;
		}

		for (const sas::AttributeId& required : {
			AbilityData::ScorchDrive::Attribute::Damage,
			AbilityData::ScorchDrive::Attribute::SegmentSpawnDistance,
			AbilityData::ScorchDrive::Attribute::BaseSegmentLifetime,
			AbilityData::ScorchDrive::Attribute::FireTickInterval,
			AbilityData::ScorchDrive::Attribute::BurnThresholdTicks,
			AbilityData::ScorchDrive::Attribute::BurnDuration,
			AbilityData::ScorchDrive::Attribute::BurnTickInterval,
			AbilityData::ScorchDrive::Attribute::BurnDamageRatio,
			AbilityData::ScorchDrive::Attribute::ReferenceMaxHealth,
			AbilityData::ScorchDrive::Attribute::MaxHealthLifetimeScale
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
					*failureReason =
						"Scorch Drive must declare every runtime balance attribute.";
				}
				return false;
			}
		}

		const auto value = [&](const sas::AttributeId& id)
		{
			return FindValue(definition.attributes, id, 0.f);
		};
		const float burnRatio = value(AbilityData::ScorchDrive::Attribute::BurnDamageRatio);
		const float threshold = value(AbilityData::ScorchDrive::Attribute::BurnThresholdTicks);
		if (value(AbilityData::ScorchDrive::Attribute::Damage) < 0.f ||
			!IsFinitePositive(value(AbilityData::ScorchDrive::Attribute::SegmentSpawnDistance)) ||
			!IsFinitePositive(value(AbilityData::ScorchDrive::Attribute::BaseSegmentLifetime)) ||
			!IsFinitePositive(value(AbilityData::ScorchDrive::Attribute::FireTickInterval)) ||
			!IsFinitePositive(threshold) || std::round(threshold) != threshold ||
			!IsFinitePositive(value(AbilityData::ScorchDrive::Attribute::BurnDuration)) ||
			!IsFinitePositive(value(AbilityData::ScorchDrive::Attribute::BurnTickInterval)) ||
			burnRatio < 0.f || burnRatio > 1.f ||
			!IsFinitePositive(value(AbilityData::ScorchDrive::Attribute::ReferenceMaxHealth)) ||
			value(AbilityData::ScorchDrive::Attribute::MaxHealthLifetimeScale) < 0.f ||
			definition.levelProgression.size() != 14 ||
			definition.damageTags.size() != 1 ||
			definition.damageTags.front() != DamageTypeSchema::Thermal)
		{
			if (failureReason)
			{
				*failureReason = "Scorch Drive contains invalid trail, Burn or progression values.";
			}
			return false;
		}
		return true;
	}

	bool ScorchDriveAbility::Activate(GameAbilityBehaviorContext& context)
	{
		SpaceShip* ship = dynamic_cast<SpaceShip*>(&context.owner);
		World* world = context.owner.GetWorld();
		if (!ship || !world || mActive)
		{
			return false;
		}

		const sas::GameplayAttributeList values = ResolveValues(context);
		mFireDamage = std::max(
			0.f,
			FindValue(values, AbilityData::ScorchDrive::Attribute::Damage, 8.f)
		);
		mSegmentSpawnDistance = std::max(
			0.1f,
			FindValue(values, AbilityData::ScorchDrive::Attribute::SegmentSpawnDistance, 60.f)
		);
		mBaseSegmentLifetime = std::max(
			0.01f,
			FindValue(values, AbilityData::ScorchDrive::Attribute::BaseSegmentLifetime, 5.f)
		);
		mFireTickInterval = std::max(
			0.01f,
			FindValue(values, AbilityData::ScorchDrive::Attribute::FireTickInterval, 0.25f)
		);
		mBurnThresholdTicks = std::max(
			1,
			static_cast<int>(std::lround(FindValue(
				values,
				AbilityData::ScorchDrive::Attribute::BurnThresholdTicks,
				4.f
			)))
		);
		mBurnDuration = std::max(
			0.f,
			FindValue(values, AbilityData::ScorchDrive::Attribute::BurnDuration, 3.f)
		);
		mBurnTickInterval = std::max(
			0.01f,
			FindValue(values, AbilityData::ScorchDrive::Attribute::BurnTickInterval, 0.5f)
		);
		mBurnDamageRatio = std::clamp(
			FindValue(values, AbilityData::ScorchDrive::Attribute::BurnDamageRatio, 0.5f),
			0.f,
			1.f
		);
		mReferenceMaxHealth = std::max(
			0.f,
			FindValue(values, AbilityData::ScorchDrive::Attribute::ReferenceMaxHealth, 100.f)
		);
		mMaxHealthLifetimeScale = std::max(
			0.f,
			FindValue(values, AbilityData::ScorchDrive::Attribute::MaxHealthLifetimeScale, 0.01f)
		);

		mCoordinator = world->SpawnActor<ScorchDriveTrailCoordinatorActor>(
			&context.owner,
			mFireDamage,
			mFireTickInterval,
			mBurnThresholdTicks,
			mBurnDuration,
			mBurnTickInterval,
			mBurnDamageRatio,
			sas::ContentId{ context.definition.abilityId },
			context.definition.abilityTags,
			context.instance.GetResolvedDamageTags(AttachmentHostKind::Ability)
		);
		if (mCoordinator.expired())
		{
			return false;
		}

		ShipRuntimeModifier modifier;
		modifier.afterburnerEnergyDrainMultiplier = 0.f;
		ship->GetRuntimeModifiers().Set(
			AbilityData::ScorchDrive::AbilityId::Basic,
			std::move(modifier)
		);

		mLastOwnerLocation = context.owner.GetActorLocation();
		mDistanceSinceLastSegment = 0.f;
		mActive = true;
		context.abilitySystem.AddOwnedTag(AbilityData::ScorchDrive::State::Active);
		return true;
	}

	void ScorchDriveAbility::Tick(
		GameAbilityBehaviorContext& context,
		float deltaTime
	)
	{
		(void)deltaTime;
		if (!mActive || context.owner.GetIsPendingDestroy())
		{
			return;
		}
		if (const auto* participant = dynamic_cast<const PortalTransferParticipant*>(
			&context.owner); participant && participant->IsInPortalTransit())
		{
			mLastOwnerLocation = context.owner.GetActorLocation();
			mOwnerWasInPortalTransit = true;
			return;
		}
		if (mOwnerWasInPortalTransit)
		{
			// Portal relocation is not travelled distance and must not create a
			// continuous line of fire segments between its endpoints.
			mLastOwnerLocation = context.owner.GetActorLocation();
			mOwnerWasInPortalTransit = false;
			return;
		}

		const PlayerSpaceShip* playerShip =
			dynamic_cast<const PlayerSpaceShip*>(&context.owner);
		if (!playerShip || !playerShip->IsAfterburning())
		{
			return;
		}
		UpdateTrail(context);
	}

	void ScorchDriveAbility::UpdateTrail(GameAbilityBehaviorContext& context)
	{
		const sf::Vector2f currentLocation = context.owner.GetActorLocation();
		const sf::Vector2f displacement = currentLocation - mLastOwnerLocation;
		const float totalDistance = GetVectorLength(displacement);
		if (totalDistance <= 0.001f)
		{
			mLastOwnerLocation = currentLocation;
			return;
		}

		const sf::Vector2f direction = NormalizeDirection(displacement);
		float remainingDistance = totalDistance;
		float distanceFromLastPosition = 0.f;
		const float firstSegmentDistance = std::max(
			0.f,
			mSegmentSpawnDistance - mDistanceSinceLastSegment
		);

		if (remainingDistance >= firstSegmentDistance)
		{
			distanceFromLastPosition = firstSegmentDistance;
			SpawnSegment(
				context,
				mLastOwnerLocation + direction * distanceFromLastPosition,
				direction
			);
			remainingDistance -= firstSegmentDistance;
			while (remainingDistance >= mSegmentSpawnDistance)
			{
				distanceFromLastPosition += mSegmentSpawnDistance;
				SpawnSegment(
					context,
					mLastOwnerLocation + direction * distanceFromLastPosition,
					direction
				);
				remainingDistance -= mSegmentSpawnDistance;
			}
			mDistanceSinceLastSegment = remainingDistance;
		}
		else
		{
			mDistanceSinceLastSegment += remainingDistance;
		}

		mLastOwnerLocation = currentLocation;
	}

	void ScorchDriveAbility::SpawnSegment(
		GameAbilityBehaviorContext& context,
		const sf::Vector2f& location,
		const sf::Vector2f& direction
	)
	{
		const shared_ptr<ScorchDriveTrailCoordinatorActor> coordinator =
			mCoordinator.lock();
		if (!coordinator || coordinator->GetIsPendingDestroy())
		{
			return;
		}

		AbilityExecutionContext executionContext{
			&context.abilitySystem,
			&context.definition,
			nullptr,
			&context.instance
		};
		const weak_ptr<AbilityWorldActor> spawned = AbilityActorSpawner::SpawnAtLocation(
			AbilityData::ScorchDrive::Actor::FireSegment::BasicDefinitionId,
			executionContext,
			context.owner,
			location,
			direction
		);
		const shared_ptr<ScorchDriveFireSegmentActor> segment =
			std::dynamic_pointer_cast<ScorchDriveFireSegmentActor>(spawned.lock());
		if (!segment)
		{
			return;
		}

		const float maxHealth = std::max(
			0.f,
			context.abilitySystem.GetAttributes().GetCurrentValue(
				OwnerAttributeIds::MaxHealth
			)
		);
		const float bonusMaxHealth = std::max(0.f, maxHealth - mReferenceMaxHealth);
		const float lifetime = mBaseSegmentLifetime +
			bonusMaxHealth * mMaxHealthLifetimeScale;
		segment->ConfigureSegment(location, direction, lifetime, mCoordinator);
		coordinator->AddSegment(segment);
	}

	void ScorchDriveAbility::End(
		GameAbilityBehaviorContext& context,
		sas::AbilityEndReason reason
	)
	{
		(void)reason;
		if (!mActive)
		{
			return;
		}

		if (const shared_ptr<ScorchDriveTrailCoordinatorActor> coordinator = mCoordinator.lock())
		{
			coordinator->SetCastActive(false);
		}
		if (SpaceShip* ship = dynamic_cast<SpaceShip*>(&context.owner))
		{
			ship->GetRuntimeModifiers().Remove(
				AbilityData::ScorchDrive::AbilityId::Basic
			);
		}
		context.abilitySystem.RemoveOwnedTag(AbilityData::ScorchDrive::State::Active);
		mCoordinator.reset();
		mDistanceSinceLastSegment = 0.f;
		mOwnerWasInPortalTransit = false;
		mActive = false;
	}
}
