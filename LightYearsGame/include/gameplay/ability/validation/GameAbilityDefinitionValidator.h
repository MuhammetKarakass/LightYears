#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/validation/AbilityActionValidator.h"

#include <string>

namespace ly
{
	// Project-level ability contract validation. It complements the structural
	// validation supplied by SpaceAbilitySystem with LightYears content rules.
	class GameAbilityDefinitionValidator final
	{
	public:
		static bool Validate(
			const GameAbilityDefinition& definition,
			const AbilityActionValidator::EffectValidationFunction& validateEffect,
			std::string* failureReason = nullptr
		);

		static bool ValidateCatalog(
			const List<const GameAbilityDefinition*>& definitions,
			const AbilityActionValidator::EffectValidationFunction& validateEffect,
			std::string* failureReason = nullptr
		);
	};
}
