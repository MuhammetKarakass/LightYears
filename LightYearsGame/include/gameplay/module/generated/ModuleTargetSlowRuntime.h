#pragma once
#include "gameplay/module/ModuleResolver.h"
#include "framework/Delegate.h"
#include "abilities/AbilityEvent.h"
#include "effects/GameplayEffectHandle.h"

namespace ly
{
	class LightYearsAbilitySystemComponent;
	class SpaceShip;

	// Toppler: an owner hit that lands on a sufficiently large target slows that target through a movement
	// slow effect. One effect per (owner, target) pair; a hit inside the window refreshes it, never stacks it.
	// Effects are removed when their module instance leaves the ship or the runtime is destroyed.
	class ModuleTargetSlowRuntime final
	{
	public:
		explicit ModuleTargetSlowRuntime(LightYearsAbilitySystemComponent& abilities);
		~ModuleTargetSlowRuntime();
		ModuleTargetSlowRuntime(const ModuleTargetSlowRuntime&) = delete;
		ModuleTargetSlowRuntime& operator=(const ModuleTargetSlowRuntime&) = delete;

		void Replace(const List<ResolvedModuleTargetSlowRule>& rules);

	private:
		struct Slowed
		{
			std::uint32_t instanceId = 0;
			weak_ptr<Actor> target;
			sas::GameplayEffectHandle handle;
		};

		void OnEvent(const sas::AbilityEvent& event);
		void RemoveEffect(const Slowed& slowed);

		LightYearsAbilitySystemComponent& mAbilities;
		SpaceShip* mShip = nullptr;
		DelegateHandle mEventHandle;
		List<ResolvedModuleTargetSlowRule> mRules;
		List<Slowed> mSlowed;
		bool mShuttingDown = false;
	};
}
