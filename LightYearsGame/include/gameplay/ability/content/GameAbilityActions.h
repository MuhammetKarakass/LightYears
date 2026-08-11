#pragma once

#include "framework/Core.h"
#include "abilities/AbilityTrigger.h"
#include "abilities/AbilityTypes.h"
#include "content/ContentId.h"
#include "effects/GameplayEffectPolicies.h"
#include "gameConfigs/combat/WeaponStructs.h"

#include <optional>
#include <string>
#include <variant>

namespace ly
{
	struct ApplyEffectAction
	{
		sas::ContentId effectId;
		sas::AbilityTargetPolicy targetPolicy = sas::AbilityTargetPolicy::Self;
	};

	// Generic metadata query for removing active effects. A cleanse is expressed
	// as data (for example Harmful + cleanseable) instead of a one-off system or
	// a hard-coded list of Slow/Ignite/Curse IDs.
	struct RemoveEffectsAction
	{
		std::optional<sas::GameplayEffectDisposition> disposition;
		bool cleanseableOnly = false;
		std::string category;
		std::string immunityCategory;
		sas::AbilityTargetPolicy targetPolicy = sas::AbilityTargetPolicy::Self;
	};

	struct SpawnActorAction
	{
		sas::ContentId actorDefinitionId;
		sas::AbilitySpawnPolicy spawnPolicy = sas::AbilitySpawnPolicy::AtOwner;
		// Spawn position and launch direction are deliberately independent. A
		// projectile can start in front of its owner while still aiming at cursor.
		sas::AbilityDirectionPolicy directionPolicy = sas::AbilityDirectionPolicy::OwnerForward;
	};

	struct FireWeaponAction
	{
		PrimaryWeaponDefinition weaponDefinition;
	};

	struct ApplyImpulseAction
	{
		float magnitude = 0.f;
		sas::AbilityDirectionPolicy directionPolicy = sas::AbilityDirectionPolicy::OwnerForward;
	};

	struct EmitGameplayEventAction
	{
		GameplayTag eventTag;
		float magnitude = 0.f;
		List<GameplayTag> payloadTags;
	};

	using AbilityActionData = std::variant<
		ApplyEffectAction,
		RemoveEffectsAction,
		SpawnActorAction,
		FireWeaponAction,
		ApplyImpulseAction,
		EmitGameplayEventAction
	>;

	struct AbilityActionSpec
	{
		sas::AbilityActionPhase phase = sas::AbilityActionPhase::OnActivate;
		AbilityActionData action;
		float interval = 0.f;
		int maxExecutions = 1;
	};

	struct AbilityTriggerSpec : sas::AbilityTrigger
	{
		List<AbilityActionSpec> actions;
	};
}
