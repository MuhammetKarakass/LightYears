#pragma once

#include "gameplay/weapon/PrimaryWeaponValidationContract.h"

namespace ly::PrimaryWeaponDefinitionValidator
{
	PrimaryWeaponValidationResult Validate(
		const PrimaryWeaponDefinition& definition
	);
}
