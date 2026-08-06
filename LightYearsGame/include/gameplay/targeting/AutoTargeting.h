#pragma once

#include "gameplay/targeting/TargetingTypes.h"

namespace ly
{
	namespace targeting
	{
		class AutoTargeting final
		{
		public:
			static List<TargetingCandidate> FindTargets(
				World& world,
				const TargetingQuery& query
			);

			// This overload keeps selection logic testable and lets callers provide
			// candidates from a subsystem other than World::GetActorsByType.
			static List<TargetingCandidate> FindTargets(
				const List<weak_ptr<Actor>>& candidates,
				const TargetingQuery& query
			);

			static weak_ptr<Actor> FindTarget(
				World& world,
				const TargetingQuery& query
			);

			static weak_ptr<Actor> FindTarget(
				const List<weak_ptr<Actor>>& candidates,
				const TargetingQuery& query
			);

			// Checks only source/geometry/filter validity. It deliberately does not
			// apply the query's selector or maxTargets limit, which makes it suitable
			// for validating an already locked target.
			static bool IsValidTarget(
				const Actor& candidate,
				const TargetingQuery& query
			);
		};

		class TargetLock final
		{
		public:
			weak_ptr<Actor> Acquire(World& world, const TargetingQuery& query);
			weak_ptr<Actor> Update(
				World& world,
				const TargetingQuery& query,
				TargetLockMode mode
			);

			void Clear();
			bool HasTarget() const;
			weak_ptr<Actor> GetTarget() const { return mTarget; }

		private:
			weak_ptr<Actor> mTarget;
		};
	}
}
