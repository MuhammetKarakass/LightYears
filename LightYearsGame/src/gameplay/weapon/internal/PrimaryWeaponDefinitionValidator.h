#pragma once

#include "gameplay/weapon/PrimaryWeaponValidationContract.h"

namespace vega::PrimaryWeaponDefinitionValidator
{
	PrimaryWeaponValidationResult Validate(
		const PrimaryWeaponDefinition& definition
	);
}
