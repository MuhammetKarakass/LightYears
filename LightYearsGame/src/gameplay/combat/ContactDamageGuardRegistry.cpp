#include "gameplay/combat/ContactDamageGuardRegistry.h"

#include <algorithm>
#include <limits>

namespace ly
{
	ContactDamageGuardHandle ContactDamageGuardRegistry::Register(Guard guard)
	{
		if (!guard)
		{
			return {};
		}

		const uint64_t firstCandidate = mNextId == 0 ? 1 : mNextId;
		uint64_t candidate = firstCandidate;
		do
		{
			const bool inUse = std::any_of(
				mGuards.begin(),
				mGuards.end(),
				[candidate](const Entry& entry)
				{
					return entry.id == candidate;
				}
			);
			if (!inUse)
			{
				mGuards.push_back(Entry{ candidate, std::move(guard) });
				mNextId = candidate == std::numeric_limits<uint64_t>::max()
					? 1
					: candidate + 1;
				return ContactDamageGuardHandle{ candidate };
			}

			candidate = candidate == std::numeric_limits<uint64_t>::max()
				? 1
				: candidate + 1;
		}
		while (candidate != firstCandidate);

		return {};
	}

	bool ContactDamageGuardRegistry::Unregister(ContactDamageGuardHandle handle)
	{
		if (!handle.IsValid())
		{
			return false;
		}

		const auto iterator = std::find_if(
			mGuards.begin(),
			mGuards.end(),
			[handle](const Entry& entry)
			{
				return entry.id == handle.id;
			}
		);
		if (iterator == mGuards.end())
		{
			return false;
		}

		mGuards.erase(iterator);
		return true;
	}

	bool ContactDamageGuardRegistry::Allows(
		const Actor& source,
		const Actor& target
	) const
	{
		for (const Entry& entry : mGuards)
		{
			if (!entry.guard(source, target))
			{
				return false;
			}
		}
		return true;
	}

	void ContactDamageGuardRegistry::Clear()
	{
		mGuards.clear();
	}

	bool ContactDamageGuardRegistry::IsEmpty() const
	{
		return mGuards.empty();
	}
}
