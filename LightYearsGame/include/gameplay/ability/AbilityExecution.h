#pragma once

#include "framework/Core.h"
#include "gameConfigs/ability/AbilityStructs.h"
#include "gameplay/weapon/PrimaryWeaponHandler.h"

namespace ly
{
	class Actor;

	struct FireWeaponRuntimeState
	{
		GameplayAttributeList runtimeAttributes;
		GameplayAttributeList resolvedAttributes;
		uint64_t resolvedAttributeRevision = 0;
		uint64_t resolvedAttachmentRevision = 0;
		bool hasResolvedAttributes = false;
		float intervalRemaining = 0.f;
		int executionCount = 0;
		bool initialized = false;
		bool lifecycleStarted = false;
		PrimaryWeaponRuntimeState weaponRuntime;
		PrimaryWeaponRuntimeState* persistentWeaponRuntime = nullptr;

		PrimaryWeaponRuntimeState& GetWeaponRuntime()
		{
			return persistentWeaponRuntime ? *persistentWeaponRuntime : weaponRuntime;
		}
	};

	struct RepeatedActionRuntimeState
	{
		float intervalAccumulator = 0.f;
		int executionCount = 0;
	};

	using AbilityActionRuntimeState = std::variant<
		std::monostate,
		FireWeaponRuntimeState,
		RepeatedActionRuntimeState
	>;

	struct ActiveAbilityAction
	{
		const AbilityActionSpec* spec = nullptr;
		AbilityActionRuntimeState runtimeState;
	};

	struct AbilityExecution
	{
		List<ActiveAbilityAction> actions;
	};
}


