#include "gameConfigs/ability/control/GravityAnomalyConfig.h"
#include "gameConfigs/ability/offensive/InfernoSprayConfig.h"
#include "gameConfigs/ability/offensive/OverdriveCoreConfig.h"
#include "gameConfigs/ability/offensive/OrbitalDronesConfig.h"
#include "gameConfigs/ability/offensive/ExecutionDriveConfig.h"
#include "gameConfigs/ability/control/NullPulseConfig.h"
#include "gameConfigs/ability/offensive/RocketConfig.h"
#include "gameConfigs/ability/offensive/SunBeamConfig.h"
#include "gameConfigs/ability/movement/DashConfig.h"
#include "gameConfigs/ability/defensive/ShieldConfig.h"
#include "gameConfigs/ability/movement/PhaseDriftConfig.h"
#include "gameConfigs/ability/offensive/HullShockConfig.h"
#include "gameConfigs/ability/defensive/ShieldHarvestConfig.h"
#include "gameConfigs/ability/utility/RelayPrismConfig.h"
#include "gameConfigs/ability/utility/EchoProtocolConfig.h"
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
			&Definitions::InfernoSpray_Basic,
			&Definitions::OverdriveCore_Basic,
			&Definitions::NullPulse_Basic,
			&Definitions::PhaseDrift_Basic,
			&Definitions::ShieldHarvest_Basic,
			&Definitions::HullShock_Basic,
			&Definitions::OrbitalDrones_Basic,
			&Definitions::ExecutionDrive_Basic,
			&Definitions::RelayPrism_Basic,
			&Definitions::EchoProtocol_Basic
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
			&OverdriveCore::ActorProjectileBasic,
			&InfernoSpray::ActorFlameConeBasic,
			&SunBeam::ActorStrikeBasic,
			&RelayPrism::ActorRelayBasic
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
			if (definition && definition->actorDefinitionId.ToString() == actorDefinitionId)
			{
				return definition;
			}
		}
		return nullptr;
	}
}
