#pragma once

#include "gameplay/weapon/PrimaryWeaponHandler.h"

namespace ly::PrimaryWeaponDefinitionValidator
{
	PrimaryWeaponValidationResult Validate(
		const PrimaryWeaponDefinition& definition
	);
}
