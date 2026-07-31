#pragma once

#include "abilities/AbilityActionScheduler.h"
#include "abilities/AbilityEvent.h"
#include "abilities/AbilityExecution.h"
#include "abilities/AbilityPolicies.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/weapon/PrimaryWeaponHandler.h"

#include <cstdint>
#include <variant>

namespace ly
{
	class GameAbility;
	class LightYearsAbilitySystemComponent;

	struct FireWeaponRuntimeState
	{
		sas::GameplayAttributeList runtimeAttributes;
		sas::GameplayAttributeList resolvedAttributes;
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
			return persistentWeaponRuntime
				? *persistentWeaponRuntime
				: weaponRuntime;
		}
	};

	using AbilityActionRuntimeState = std::variant<
		std::monostate,
		FireWeaponRuntimeState,
		sas::RepeatedAbilityActionState
	>;
	using ActiveAbilityAction =
		sas::ActiveAbilityAction<AbilityActionSpec, AbilityActionRuntimeState>;
	using GameAbilityExecution =
		sas::AbilityExecution<ActiveAbilityAction>;

	struct AbilityExecutionContext
	{
		LightYearsAbilitySystemComponent* abilitySystem = nullptr;
		const GameAbilityDefinition* definition = nullptr;
		const sas::AbilityEvent* event = nullptr;
		GameAbility* instance = nullptr;
	};

	class GameAbilityActionExecutor
	{
	public:
		static void BeginExecution(
			GameAbilityExecution& execution,
			AbilityExecutionContext& context
		);
		static void TickExecution(
			GameAbilityExecution& execution,
			AbilityExecutionContext& context,
			float deltaTime
		);
		static void EndExecution(
			GameAbilityExecution& execution,
			AbilityExecutionContext& context,
			sas::AbilityEndReason reason
		);

	private:
		static void TickAction(
			ActiveAbilityAction& action,
			AbilityExecutionContext& context,
			float deltaTime
		);
		static void ExecuteAction(
			ActiveAbilityAction& action,
			AbilityExecutionContext& context
		);
	};
}
