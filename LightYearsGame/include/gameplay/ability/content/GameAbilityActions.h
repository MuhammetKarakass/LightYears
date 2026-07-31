#pragma once

#include "framework/Core.h"
#include "abilities/AbilityTrigger.h"
#include "abilities/AbilityTypes.h"
#include "gameConfigs/combat/WeaponStructs.h"

#include <string>
#include <variant>

namespace ly
{
	struct ApplyEffectAction
	{
		std::string effectId;
		sas::AbilityTargetPolicy targetPolicy = sas::AbilityTargetPolicy::Self;
	};

	struct SpawnActorAction
	{
		std::string actorDefinitionId;
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
	};

	using AbilityActionData = std::variant<
		ApplyEffectAction,
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
