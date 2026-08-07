#pragma once

#include "effects/GameplayEffectDefinition.h"

#include <functional>
#include <string>

namespace ly
{
	// LightYears-specific semantic validation for effect content. The generic
	// sas validation remains in SpaceAbilitySystem; this layer owns project tags,
	// presentation registration, and behavior registration rules.
	class GameplayEffectDefinitionValidator final
	{
	public:
		using BehaviorRegistrationQuery = std::function<bool(const GameplayTag&)>;

		static bool Validate(
			const sas::GameplayEffectDefinition& definition,
			const BehaviorRegistrationQuery& isBehaviorRegistered,
			std::string* failureReason = nullptr
		);

		static bool ValidateCatalog(
			const List<const sas::GameplayEffectDefinition*>& definitions,
			const BehaviorRegistrationQuery& isBehaviorRegistered,
			std::string* failureReason = nullptr
		);
	};
}
