#include "gameplay/ability/energySpear/EnergySpearContentRegistration.h"

#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/content/AbilityBehaviorType.h"
#include "gameplay/ability/energySpear/EnergySpearAbility.h"

#include <memory>

namespace ly
{
	bool RegisterEnergySpearAbilityBehavior()
	{
		return GameAbilityBehaviorRegistry::Register(
			AbilityBehaviorType::EnergySpear,
			[] { return std::make_unique<EnergySpearAbility>(); }
		);
	}
}
