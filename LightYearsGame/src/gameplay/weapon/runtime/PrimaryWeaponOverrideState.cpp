#include "gameplay/weapon/runtime/PrimaryWeaponOverrideState.h"

#include <algorithm>

namespace ly
{
	PrimaryWeaponOverrideHandle PrimaryWeaponOverrideState::Push(
		const sas::ContentId& sourceId,
		const PrimaryWeaponDefinition& weaponDefinition,
		int priority
	)
	{
		if (weaponDefinition.weaponId.empty())
		{
			return 0;
		}
		const PrimaryWeaponOverrideHandle handle = mNextHandle++;
		mOverrides.push_back(ActiveOverride{ handle, sourceId, weaponDefinition, priority });
		return handle;
	}

	bool PrimaryWeaponOverrideState::Remove(PrimaryWeaponOverrideHandle handle)
	{
		const auto found = std::find_if(mOverrides.begin(), mOverrides.end(),
			[handle](const ActiveOverride& entry) { return entry.handle == handle; });
		if (found == mOverrides.end()) return false;
		mOverrides.erase(found);
		return true;
	}

	void PrimaryWeaponOverrideState::Clear() { mOverrides.clear(); }

	PrimaryWeaponOverrideState::ActiveOverride* PrimaryWeaponOverrideState::GetActive()
	{
		const auto found = FindActive();
		return found == mOverrides.end() ? nullptr : &*found;
	}

	const PrimaryWeaponOverrideState::ActiveOverride* PrimaryWeaponOverrideState::GetActive() const
	{
		const auto found = FindActive();
		return found == mOverrides.end() ? nullptr : &*found;
	}

	std::vector<PrimaryWeaponOverrideState::ActiveOverride>::iterator
	PrimaryWeaponOverrideState::FindActive()
	{
		return std::max_element(mOverrides.begin(), mOverrides.end(),
			[](const ActiveOverride& left, const ActiveOverride& right)
			{
				// Equal priorities are LIFO so a nested temporary form restores the
				// exact override that was active before it.
				return left.priority < right.priority ||
					(left.priority == right.priority && left.handle < right.handle);
			});
	}

	std::vector<PrimaryWeaponOverrideState::ActiveOverride>::const_iterator
	PrimaryWeaponOverrideState::FindActive() const
	{
		return std::max_element(mOverrides.begin(), mOverrides.end(),
			[](const ActiveOverride& left, const ActiveOverride& right)
			{
				return left.priority < right.priority ||
					(left.priority == right.priority && left.handle < right.handle);
			});
	}
}
