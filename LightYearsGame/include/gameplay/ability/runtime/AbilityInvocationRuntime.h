#pragma once

#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/runtime/AbilityUseHistory.h"

#include <cstddef>
#include <string>
#include <vector>

namespace ly
{
	class LightYearsAbilitySystemComponent;

	// A temporary ability instance executes through the same behavior, action,
	// targeting and actor-spawn pipeline as a normal granted ability. The
	// invocation runtime owns only these short-lived instances; it never puts
	// them in the player's loadout registry.
	class AbilityInvocationRuntime final
	{
	public:
		explicit AbilityInvocationRuntime(
			LightYearsAbilitySystemComponent& abilitySystem
		);

		bool Invoke(
			const AbilityUseRecord& record,
			const List<sas::AttributeScalingRule>& scalingRules,
			float outputMultiplier,
			sas::AbilitySlot controlSlot,
			bool inputHeld,
			std::string* failureReason = nullptr
		);

		// The controlling input is updated by the normal loadout input path, so
		// echoed WhileHeld abilities can charge and release like their originals.
		void SetControlInput(sas::AbilitySlot controlSlot, bool inputHeld);
		void Tick(float deltaTime);
		void Clear();

		std::size_t GetActiveInvocationCount() const
		{
			return mActiveInvocations.size();
		}

	private:
		static GameAbilityDefinition BuildInvocationDefinition(
			const GameAbilityDefinition& sourceDefinition,
			const AbilityUseRecord& record,
			const List<sas::AttributeScalingRule>& scalingRules,
			float outputMultiplier
		);

		struct ActiveInvocation
		{
			unique_ptr<GameAbility> ability;
			sas::AbilitySlot controlSlot = sas::AbilitySlot::None;
		};

		LightYearsAbilitySystemComponent& mAbilitySystem;
		std::vector<ActiveInvocation> mActiveInvocations;
	};
}
