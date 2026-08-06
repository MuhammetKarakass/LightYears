#include "gameConfigs/ability/offensive/GravityAnomalyConfig.h"
#include "gameConfigs/ability/offensive/InfernoSprayConfig.h"
#include "gameConfigs/ability/offensive/RocketConfig.h"
#include "gameConfigs/ability/offensive/SunBeamConfig.h"
#include "gameConfigs/ability/functional/DashConfig.h"
#include "gameConfigs/ability/defensive/ShieldConfig.h"
#include "gameplay/content/AbilityContentCatalog.h"

namespace AbilityData
{
	const ly::List<const ly::GameAbilityDefinition*>& GetBuiltinShippedAbilityDefinitions()
	{
		static const ly::List<const ly::GameAbilityDefinition*> definitions{
			&Definitions::Shield_Basic,
			&Definitions::SunBeam_Strike_Basic,
			&Definitions::Dash_Basic,
			&Definitions::GravityAnomaly_Basic,
			&Definitions::Rocket_Basic,
			&Definitions::InfernoSpray_Basic
		};
		return definitions;
	}

	const ly::List<const ly::GameAbilityDefinition*>& GetShippedAbilityDefinitions()
	{
		if (!ly::content::AbilityContentCatalog::IsLoaded())
		{
			return GetBuiltinShippedAbilityDefinitions();
		}

		return ly::content::AbilityContentCatalog::GetDefinitions();
	}

	const ly::List<const ly::AbilityActorDefinition*>& GetBuiltinAbilityActorDefinitions()
	{
		static const ly::List<const ly::AbilityActorDefinition*> definitions{
			&GravityAnomaly::ActorProjectileBasic,
			&GravityAnomaly::ActorFieldBasic,
			&Rocket::ActorProjectileBasic,
			&InfernoSpray::ActorFlameConeBasic,
			&SunBeam::ActorStrikeBasic
		};
		return definitions;
	}

	const ly::GameAbilityDefinition* FindShippedAbilityDefinition(
		const std::string& abilityId
	)
	{
		if (ly::content::AbilityContentCatalog::IsLoaded())
		{
			return ly::content::AbilityContentCatalog::FindById(abilityId);
		}

		for (const ly::GameAbilityDefinition* definition : GetBuiltinShippedAbilityDefinitions())
		{
			if (definition && definition->abilityId == abilityId)
			{
				return definition;
			}
		}
		return nullptr;
	}

	const ly::AbilityActorDefinition* FindAbilityActorDefinition(
		const std::string& actorDefinitionId)
	{
		if (ly::content::AbilityContentCatalog::IsLoaded())
		{
			return ly::content::AbilityContentCatalog::FindActorById(actorDefinitionId);
		}

		for (const ly::AbilityActorDefinition* definition : GetBuiltinAbilityActorDefinitions())
		{
			if (definition && definition->actorDefinitionId == actorDefinitionId)
			{
				return definition;
			}
		}
		return nullptr;
	}
}
