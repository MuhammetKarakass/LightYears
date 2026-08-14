#pragma once

#include "abilities/AbilityEvent.h"
#include "content/ContentId.h"
#include "framework/Core.h"

#include <functional>
#include <optional>
#include <vector>

namespace ly
{
	// Filters keep observers reusable: a future system can listen to one
	// ability, one lifecycle event, one activation origin, or any combination
	// without adding another callback field to the ability component.
	struct AbilityLifecycleObserverFilter
	{
		std::optional<sas::ContentId> abilityId;
		std::optional<GameplayTag> eventTag;
		std::optional<sas::AbilityActivationOrigin> activationOrigin;
		std::optional<sas::AbilitySlot> slot;

		bool Matches(const sas::AbilityLifecycleEvent& event) const;
	};

	struct AbilityLifecycleObserverHandle
	{
		std::size_t id = 0;

		bool IsValid() const { return id != 0; }
	};

	struct AbilityActivationGuardHandle
	{
		std::size_t id = 0;

		bool IsValid() const { return id != 0; }
	};

	// A guard is intentionally separate from an observer. Observers report
	// state; guards are the only callbacks allowed to veto an activation.
	using AbilityLifecycleObserver =
		std::function<void(const sas::AbilityLifecycleEvent&)>;
	using AbilityActivationGuard =
		std::function<bool(const sas::AbilityLifecycleEvent&)>;

	class AbilityLifecycleDispatcher
	{
	public:
		AbilityLifecycleObserverHandle RegisterObserver(
			AbilityLifecycleObserverFilter filter,
			AbilityLifecycleObserver callback,
			int priority = 0
		);
		bool UnregisterObserver(AbilityLifecycleObserverHandle handle);

		AbilityActivationGuardHandle RegisterActivationGuard(
			AbilityActivationGuard callback,
			int priority = 0
		);
		bool UnregisterActivationGuard(AbilityActivationGuardHandle handle);

		// Guards are evaluated before the behavior's Activate() method. A false
		// result rejects the activation without starting cooldown or execution.
		bool CanActivate(const sas::AbilityLifecycleEvent& event) const;

		// The registry takes a snapshot before dispatch. A callback may therefore
		// register or unregister another callback without invalidating iteration;
		// such changes take effect on the next dispatch.
		void Publish(const sas::AbilityLifecycleEvent& event) const;

		void Clear();

	private:
		struct ObserverEntry
		{
			AbilityLifecycleObserverHandle handle;
			AbilityLifecycleObserverFilter filter;
			AbilityLifecycleObserver callback;
			int priority = 0;
		};

		struct GuardEntry
		{
			AbilityActivationGuardHandle handle;
			AbilityActivationGuard callback;
			int priority = 0;
		};

		std::vector<ObserverEntry> mObservers;
		std::vector<GuardEntry> mGuards;
		std::size_t mNextHandle = 1;
	};
}
