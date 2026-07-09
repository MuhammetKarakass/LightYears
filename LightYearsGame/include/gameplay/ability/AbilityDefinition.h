#pragma once

#include "framework/Core.h"
#include <string>

namespace ly
{
	enum class AbilitySlot
	{
		PrimaryFire,
		Skill1,
		Skill2,
		Skill3,
		Skill4
	};

	enum class AbilityActivationPolicy
	{
		OnPressed,
		WhileHeld,
		Toggle,
		Passive
	};

	enum class AbilityControllerType
	{
		PrimaryWeapon,
		ProjectileFire,
		Beam,
		Shield,
		Dash,
		Boost
	};

	struct AbilityDefinition
	{
		std::string abilityId;
		AbilitySlot slot;
		AbilityControllerType controllerType;
		AbilityActivationPolicy activationPolicy;
		float cooldown;
		float duration;
		int maxCharges;
		List<GameplayTag> tags;

		AbilityDefinition(
			const std::string& inAbilityId = "Ability",
			AbilitySlot inSlot = AbilitySlot::Skill1,
			AbilityControllerType inControllerType = AbilityControllerType::ProjectileFire,
			AbilityActivationPolicy inActivationPolicy = AbilityActivationPolicy::OnPressed,
			float inCooldown = 0.f,
			float inDuration = 0.f,
			int inMaxCharges = 0,
			const List<GameplayTag>& inTags = {}
		)
			: abilityId{ inAbilityId },
			slot{ inSlot },
			controllerType{ inControllerType },
			activationPolicy{ inActivationPolicy },
			cooldown{ inCooldown },
			duration{ inDuration },
			maxCharges{ inMaxCharges },
			tags{ inTags }
		{
		}
	};
}
