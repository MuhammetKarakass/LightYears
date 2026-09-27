#include "gameplay/ability/aegisReaver/AegisReaverAbility.h"

#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/GameAbilityActionExecutor.h"
#include "gameplay/ability/actors/AbilityActorSpawner.h"
#include "gameplay/ability/aegisReaver/AegisReaverContracts.h"
#include "gameplay/ability/aegisReaver/AegisReaverFlightState.h"
#include "gameplay/ability/aegisReaver/AegisReaverProjectileActor.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "spaceShip/SpaceShip.h"

#include <algorithm>
#include <memory>

namespace ly
{
	bool AegisReaverAbility::Activate(GameAbilityBehaviorContext& context)
	{
		auto* owner = dynamic_cast<SpaceShip*>(&context.owner);
		if (!owner || !owner->GetWorld())
		{
			return false;
		}

		const float sacrificedShield = std::max(
			0.f,
			owner->GetShieldComponent().GetShield()
		);
		auto flightState = std::make_shared<AegisReaverFlightState>(*owner);
		AbilityExecutionContext executionContext{
			&context.abilitySystem,
			&context.definition,
			nullptr,
			&context.instance
		};
		const SpawnActorAction spawnAction{
			AbilityData::AegisReaver::Actor::Projectile::BasicDefinitionId,
			sas::AbilitySpawnPolicy::OwnerForward,
			sas::AbilityDirectionPolicy::MouseWorld
		};
		const shared_ptr<AegisReaverProjectileActor> projectile =
			std::dynamic_pointer_cast<AegisReaverProjectileActor>(
				AbilityActorSpawner::Spawn(spawnAction, executionContext, context.owner).lock()
			);
		if (!projectile || !projectile->PrepareFlight(flightState, sacrificedShield))
		{
			return false;
		}

		owner->GetShieldComponent().ChangeShield(-sacrificedShield);
		return true;
	}

	bool AegisReaverAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		const AbilityActorDefinition* actor = AbilityData::FindAbilityActorDefinition(
			AbilityData::AegisReaver::Actor::Projectile::BasicDefinitionId
		);
		const bool hasRequiredActor = actor &&
			sas::FindAttribute(actor->attributes, CommonAttributeIds::Damage) &&
			sas::FindAttribute(actor->attributes, CommonAttributeIds::Range) &&
			sas::FindAttribute(actor->attributes, CollisionAttributeIds::Radius) &&
			sas::FindAttribute(
				actor->attributes,
				AbilityData::AegisReaver::Actor::Projectile::ProjectileSpeed
			) &&
			sas::FindAttribute(
				actor->attributes,
				AbilityData::AegisReaver::Actor::Projectile::ReturnSpeed
			) &&
			sas::FindAttribute(
				actor->attributes,
				AbilityData::AegisReaver::Actor::Projectile::ShieldConversionRatio
			) &&
			sas::FindAttribute(
				actor->attributes,
				AbilityData::AegisReaver::Actor::Projectile::ShieldStealRatio
			);
		if (!hasRequiredActor ||
			definition.activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
			definition.lifetimePolicy != sas::AbilityLifetimePolicy::Instant ||
			!definition.actions.empty() ||
			definition.maxCharges != 1 || definition.cooldown <= 0.f ||
			definition.damageTags.size() != 1 ||
			definition.damageTags.front() != DamageTypeSchema::Energy)
		{
			if (failureReason)
			{
				*failureReason =
					"Aegis Reaver requires an instant Energy projectile and its complete runtime attribute set. "
					"Actor=" + std::to_string(hasRequiredActor) +
					", actions=" + std::to_string(definition.actions.size()) +
					", charges=" + std::to_string(definition.maxCharges) +
					", cooldown=" + std::to_string(definition.cooldown) +
					", damageTags=" + std::to_string(definition.damageTags.size());
			}
			return false;
		}
		return true;
	}
}
