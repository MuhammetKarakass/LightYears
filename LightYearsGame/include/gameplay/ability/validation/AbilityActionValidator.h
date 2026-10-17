#pragma once

#include "effects/GameplayEffectDefinition.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"

#include <functional>
#include <string>

namespace vega
{
	// Action validation depends on LightYears registries (weapon, actor, effect)
	// but has no component state. Keeping it separate prevents GameAbility
	// definition validation from becoming another monolithic implementation.
	class AbilityActionValidator final
	{
	public:
		using EffectLookup = std::function<const sas::GameplayEffectDefinition*(const std::string&)>;
		using EffectValidationFunction = std::function<bool(
			const sas::GameplayEffectDefinition&,
			std::string*
		)>;

		static bool Validate(
			const List<AbilityActionSpec>& actions,
			const EffectValidationFunction& validateEffect,
			std::string* failureReason = nullptr,
			const EffectLookup& findEffect = {}
		);
	};
}
