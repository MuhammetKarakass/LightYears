#include "gameplay/ability/AbilityBehaviorRegistration.h"

#include "gameConfigs/ability/DashConfig.h"
#include "gameConfigs/ability/GravityAnomalyConfig.h"
#include "gameConfigs/ability/RocketConfig.h"
#include "gameConfigs/ability/ShieldConfig.h"
#include "gameConfigs/ability/SunBeamConfig.h"
#include "gameplay/ability/AbilityBehaviorRegistry.h"
#include "gameplay/ability/dash/DashAbility.h"
#include "gameplay/ability/gravityAnomaly/GravityAnomalyAbility.h"
#include "gameplay/ability/gravityAnomaly/GravityAnomalyFieldActor.h"
#include "gameplay/ability/gravityAnomaly/GravityAnomalyProjectileActor.h"
#include "gameplay/ability/rocket/RocketProjectileActor.h"
#include "gameplay/ability/rocket/RocketAbility.h"
#include "gameplay/ability/shield/ShieldAbility.h"
#include "gameplay/ability/sunBeam/SunBeamAbility.h"
#include "gameplay/ability/sunBeam/SunBeamStrikeActor.h"
#include "presentation/ability/AbilityPresentationContent.h"

namespace ly
{
	bool RegisterGameAbilityBehaviors()
	{
		static const bool registered = []
		{
			const bool dashRegistered = AbilityBehaviorRegistry::Register(
				AbilityData::Dash::BehaviorId,
				[] { return std::make_unique<DashAbility>(); }
			);
			const bool shieldRegistered = AbilityBehaviorRegistry::Register(
				AbilityData::Shield::BehaviorId,
				[] { return std::make_unique<ShieldAbility>(); }
			);
			const bool gravityAnomalyRegistered = AbilityBehaviorRegistry::Register(
				AbilityData::GravityAnomaly::BehaviorId,
				[] { return std::make_unique<GravityAnomalyAbility>(); }
			);
			const bool rocketRegistered = AbilityBehaviorRegistry::Register(
				AbilityData::Rocket::BehaviorId,
				[] { return std::make_unique<RocketAbility>(); }
			);
			const bool sunBeamRegistered = AbilityBehaviorRegistry::Register(
				AbilityData::SunBeam::BehaviorId,
				[] { return std::make_unique<SunBeamAbility>(); }
			);
			return dashRegistered && shieldRegistered && gravityAnomalyRegistered &&
				rocketRegistered && sunBeamRegistered;
		}();
		return registered;
	}

	bool RegisterGameAbilityActorTypes()
	{
		static const bool registered =
			RegisterGravityAnomalyProjectileActorType() &&
			RegisterGravityAnomalyFieldActorType() &&
			RegisterRocketProjectileActorType() &&
			RegisterSunBeamStrikeActorType();
		return registered;
	}

	bool RegisterGameAbilityContent()
	{
		return RegisterGameAbilityPresentationContent()
			&& RegisterGameAbilityBehaviors()
			&& RegisterGameAbilityActorTypes();
	}
}
