#include "abilities/AbilityCooldownTracker.h"

namespace sas
{
	bool AbilityCooldownTracker::IsActive(const std::string& key) const
	{
		const auto found = mCooldowns.find(key);
		return found != mCooldowns.end() && found->second > 0.f;
	}

	void AbilityCooldownTracker::Start(const std::string& key, float duration)
	{
		if (!key.empty() && duration > 0.f)
		{
			mCooldowns[key] = duration;
		}
	}

	void AbilityCooldownTracker::Tick(float deltaTime)
	{
		for (auto it = mCooldowns.begin(); it != mCooldowns.end();)
		{
			it->second -= deltaTime;
			if (it->second <= 0.f)
			{
				it = mCooldowns.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	void AbilityCooldownTracker::Clear()
	{
		mCooldowns.clear();
	}
}
