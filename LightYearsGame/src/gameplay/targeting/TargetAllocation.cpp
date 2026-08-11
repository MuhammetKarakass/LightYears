#include "gameplay/targeting/TargetAllocation.h"

namespace ly::targeting
{
	List<weak_ptr<Actor>> TargetAllocation::DistributeEvenly(
		const List<TargetingCandidate>& candidates,
		std::size_t projectileCount
	)
	{
		List<weak_ptr<Actor>> allocation;
		if (projectileCount == 0 || candidates.empty())
		{
			return allocation;
		}

		// Candidates are already ordered by the caller's targeting policy. A
		// round-robin assignment gives every target the same number of rockets,
		// with only the unavoidable remainder going to the nearest targets.
		allocation.reserve(projectileCount);
		for (std::size_t projectileIndex = 0;
			projectileIndex < projectileCount;
			++projectileIndex)
		{
			allocation.emplace_back(candidates[projectileIndex % candidates.size()].actor);
		}
		return allocation;
	}
}
