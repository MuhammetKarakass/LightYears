#pragma once

#include "framework/Core.h"

#include <SFML/System/Vector2.hpp>
#include <cstddef>
#include <functional>

namespace ly
{
	class Actor;
	class World;

	namespace targeting
	{
		enum class TargetingShape : uint8_t
		{
			Radius,
			Cone,
			Rectangle
		};

		enum class TargetLockMode : uint8_t
		{
			// Keep the acquired target until it is destroyed. Range/filter changes do
			// not invalidate a snapshot lock.
			Snapshot,

			// Keep the target while it still satisfies the query, then clear it.
			Sticky,

			// Keep the target while valid, otherwise acquire a replacement.
			ReacquireIfInvalid,

			// Re-run the query on every update and choose the current best target.
			ReevaluateEveryRefresh
		};

		struct TargetingCandidate
		{
			shared_ptr<Actor> actor;
			float distanceSquared = 0.f;
			float angleRadians = 0.f;
		};

		struct TargetingQuery;

		using CandidateProvider = std::function<List<weak_ptr<Actor>>(
			World&,
			const TargetingQuery& query
		)>;

		using CandidateFilter = std::function<bool(
			const Actor* source,
			const Actor& candidate,
			const TargetingCandidate& metadata
		)>;

		using CandidateComparator = std::function<bool(
			const TargetingCandidate& left,
			const TargetingCandidate& right
		)>;

		using CandidateSelector = std::function<void(
			List<TargetingCandidate>& candidates,
			std::size_t maxTargets
		)>;

		struct TargetingQuery
		{
			const Actor* source = nullptr;
			sf::Vector2f origin{ 0.f, 0.f };
			float range = 0.f;
			TargetingShape shape = TargetingShape::Radius;
			sf::Vector2f direction{ 1.f, 0.f };
			float coneHalfAngleRadians = 0.f;
			// Rectangle queries use direction as their local forward axis. The
			// half-extents are expressed as (length / 2, width / 2).
			sf::Vector2f rectangleHalfExtents{ 0.f, 0.f };

			// Zero means unlimited. A custom selector may still return fewer targets.
			std::size_t maxTargets = 0;

			// These are optional broad-phase filters. More specific rules belong in
			// filter or selector policies supplied by the caller.
			CollisionLayer requiredTargetLayers = CollisionLayer::None;
			CollisionLayer requiredCandidateCollisionMask = CollisionLayer::None;
			bool requireCollisionCompatibility = false;
			bool excludeSource = true;
			List<const Actor*> excludedTargets;

			CandidateProvider candidateProvider;
			CandidateFilter filter;
			CandidateComparator comparator;
			CandidateSelector selector;
		};
	}
}
