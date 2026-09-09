#include "gameplay/ability/aegisReaver/AegisReaverAbility.h"

#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/aegisReaver/AegisReaverContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/damage/DamageTypeSystem.h"

namespace ly
{
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
			definition.maxCharges != 1 || definition.cooldown <= 0.f ||
			definition.damageTags.size() != 1 ||
			definition.damageTags.front() != DamageTypeSchema::Energy)
		{
			if (failureReason)
			{
				*failureReason =
					"Aegis Reaver requires an instant Energy projectile and its complete runtime attribute set.";
			}
			return false;
		}
		return true;
	}
}
