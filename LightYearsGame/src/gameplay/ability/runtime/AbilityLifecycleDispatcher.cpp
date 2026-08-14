#include "gameplay/ability/runtime/AbilityLifecycleDispatcher.h"

#include <algorithm>

namespace ly
{
	bool AbilityLifecycleObserverFilter::Matches(
		const sas::AbilityLifecycleEvent& event
	) const
	{
		return (!abilityId.has_value() || event.abilityId == *abilityId) &&
			(!eventTag.has_value() || event.eventTag.MatchesTag(*eventTag)) &&
			(!activationOrigin.has_value() ||
				event.activationOrigin == *activationOrigin) &&
			(!slot.has_value() || event.slot == *slot);
	}

	AbilityLifecycleObserverHandle AbilityLifecycleDispatcher::RegisterObserver(
		AbilityLifecycleObserverFilter filter,
		AbilityLifecycleObserver callback,
		int priority
	)
	{
		if (!callback)
		{
			return {};
		}

		const AbilityLifecycleObserverHandle handle{ mNextHandle++ };
		mObservers.push_back(ObserverEntry{
			handle,
			std::move(filter),
			std::move(callback),
			priority
		});
		std::stable_sort(
			mObservers.begin(),
			mObservers.end(),
			[](const ObserverEntry& left, const ObserverEntry& right)
			{
				return left.priority > right.priority;
			}
		);
		return handle;
	}

	bool AbilityLifecycleDispatcher::UnregisterObserver(
		AbilityLifecycleObserverHandle handle
	)
	{
		const auto found = std::find_if(
			mObservers.begin(),
			mObservers.end(),
			[&](const ObserverEntry& entry)
			{
				return entry.handle.id == handle.id;
			}
		);
		if (found == mObservers.end())
		{
			return false;
		}
		mObservers.erase(found);
		return true;
	}

	AbilityActivationGuardHandle AbilityLifecycleDispatcher::RegisterActivationGuard(
		AbilityActivationGuard callback,
		int priority
	)
	{
		if (!callback)
		{
			return {};
		}

		const AbilityActivationGuardHandle handle{ mNextHandle++ };
		mGuards.push_back(GuardEntry{
			handle,
			std::move(callback),
			priority
		});
		std::stable_sort(
			mGuards.begin(),
			mGuards.end(),
			[](const GuardEntry& left, const GuardEntry& right)
			{
				return left.priority > right.priority;
			}
		);
		return handle;
	}

	bool AbilityLifecycleDispatcher::UnregisterActivationGuard(
		AbilityActivationGuardHandle handle
	)
	{
		const auto found = std::find_if(
			mGuards.begin(),
			mGuards.end(),
			[&](const GuardEntry& entry)
			{
				return entry.handle.id == handle.id;
			}
		);
		if (found == mGuards.end())
		{
			return false;
		}
		mGuards.erase(found);
		return true;
	}

	bool AbilityLifecycleDispatcher::CanActivate(
		const sas::AbilityLifecycleEvent& event
	) const
	{
		const std::vector<GuardEntry> guards = mGuards;
		for (const GuardEntry& guard : guards)
		{
			if (guard.callback && !guard.callback(event))
			{
				return false;
			}
		}
		return true;
	}

	void AbilityLifecycleDispatcher::Publish(
		const sas::AbilityLifecycleEvent& event
	) const
	{
		const std::vector<ObserverEntry> observers = mObservers;
		for (const ObserverEntry& observer : observers)
		{
			if (observer.callback && observer.filter.Matches(event))
			{
				observer.callback(event);
			}
		}
	}

	void AbilityLifecycleDispatcher::Clear()
	{
		mObservers.clear();
		mGuards.clear();
	}
}
