#include "gameplay/ability/validation/AbilityActionValidator.h"

#include "gameConfigs/ability/AbilityCatalog.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "gameplay/weapon/PrimaryWeaponExecutionSystem.h"

namespace ly
{
	bool AbilityActionValidator::Validate(
		const List<AbilityActionSpec>& actions,
		const EffectValidationFunction& validateEffect,
		std::string* failureReason
	)
	{
		for (const AbilityActionSpec& action : actions)
		{
			if (const auto* fireWeapon = std::get_if<FireWeaponAction>(&action.action))
			{
				const PrimaryWeaponValidationResult validation =
					PrimaryWeaponExecutionSystem::ValidateDefinition(
						fireWeapon->weaponDefinition
					);
				if (!validation.isValid)
				{
					if (failureReason)
					{
						*failureReason = validation.reason;
					}
					return false;
				}
			}
			else if (const auto* spawnActor = std::get_if<SpawnActorAction>(&action.action))
			{
				const AbilityActorDefinition* actorDefinition =
					AbilityData::FindAbilityActorDefinition(spawnActor->actorDefinitionId);
				if (!actorDefinition)
				{
					if (failureReason)
					{
						*failureReason =
							"Spawn actor action references an unknown ability actor definition.";
					}
					return false;
				}
				const AbilityActorValidationResult validation =
					AbilityActorRegistry::ValidateDefinition(*actorDefinition);
				if (!validation.isValid)
				{
					if (failureReason)
					{
						*failureReason = validation.reason;
					}
					return false;
				}
			}
			else if (const auto* applyEffect = std::get_if<ApplyEffectAction>(&action.action))
			{
				const sas::GameplayEffectDefinition* definition =
					EffectData::FindGameplayEffectDefinition(applyEffect->effectId);
				if (!definition)
				{
					if (failureReason)
					{
						*failureReason =
							"Apply effect action references an unknown gameplay effect definition.";
					}
					return false;
				}
				if (!validateEffect || !validateEffect(*definition, failureReason))
				{
					return false;
				}
			}
		}
		return true;
	}
}
