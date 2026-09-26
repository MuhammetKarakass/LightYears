#pragma once

#include "framework/Actor.h"
#include <cstddef>
#include <unordered_map>

namespace ly::targeting
{
	class ActorHitCooldowns
	{
	public:
		void PruneExpired(float now)
		{
			for (auto entry = mExpiryByActorId.begin(); entry != mExpiryByActorId.end();)
			{
				if (entry->second <= now)
				{
					entry = mExpiryByActorId.erase(entry);
				}
				else
				{
					++entry;
				}
			}
		}

		bool IsCoolingDown(Actor& actor, float now) const
		{
			const auto entry = mExpiryByActorId.find(actor.GetUniqueID());
			return entry != mExpiryByActorId.end() && entry->second > now;
		}

		void RecordHit(Actor& actor, float expiry)
		{
			mExpiryByActorId[actor.GetUniqueID()] = expiry;
		}

		std::size_t GetEntryCount() const
		{
			return mExpiryByActorId.size();
		}

	private:
		std::unordered_map<unsigned int, float> mExpiryByActorId;
	};
}
