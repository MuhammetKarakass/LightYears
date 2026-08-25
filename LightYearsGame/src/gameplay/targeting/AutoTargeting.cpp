#include "gameplay/targeting/AutoTargeting.h"

#include "framework/Actor.h"
#include "framework/World.h"
#include "gameplay/targeting/SweptGeometry.h"

#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace ly
{
	namespace targeting
	{
		namespace
		{
			constexpr float DirectionEpsilonSquared = 0.000001f;

			float LengthSquared(const sf::Vector2f& value)
			{
				return value.x * value.x + value.y * value.y;
			}

			float Dot(const sf::Vector2f& left, const sf::Vector2f& right)
			{
				return left.x * right.x + left.y * right.y;
			}

			bool ContainsActor(const List<const Actor*>& actors, const Actor* candidate)
			{
				return std::find(actors.begin(), actors.end(), candidate) != actors.end();
			}

			bool IsInsideQueryShape(
				const sf::Vector2f& offset,
				const TargetingQuery& query,
				float& outDistanceSquared,
				float& outAngleRadians
			)
			{
				outDistanceSquared = LengthSquared(offset);
				outAngleRadians = 0.f;
				const float safeRange = std::max(0.f, query.range);
				if (outDistanceSquared > safeRange * safeRange)
				{
					return false;
				}

				if (query.shape == TargetingShape::Radius)
				{
					return true;
				}

				const float directionLengthSquared = LengthSquared(query.direction);
				if (directionLengthSquared <= DirectionEpsilonSquared)
				{
					return false;
				}

				const float offsetLengthSquared = outDistanceSquared;
				if (offsetLengthSquared <= DirectionEpsilonSquared)
				{
					outAngleRadians = 0.f;
					return true;
				}

				if (query.shape == TargetingShape::Rectangle)
				{
					const float directionLength = std::sqrt(directionLengthSquared);
					const sf::Vector2f forward = query.direction / directionLength;
					const sf::Vector2f right{ -forward.y, forward.x };
					const float forwardDistance = Dot(forward, offset);
					const float sideDistance = Dot(right, offset);
					return std::abs(forwardDistance) <=
						std::max(0.f, query.rectangleHalfExtents.x) &&
						std::abs(sideDistance) <=
						std::max(0.f, query.rectangleHalfExtents.y);
				}

				const float normalizedDot = std::clamp(
					Dot(query.direction, offset) /
						std::sqrt(directionLengthSquared * offsetLengthSquared),
					-1.f,
					1.f
				);
				outAngleRadians = std::acos(normalizedDot);
				return outAngleRadians <= std::max(0.f, query.coneHalfAngleRadians);
			}

			bool EvaluateCandidate(
				const Actor& candidate,
				const TargetingQuery& query,
				TargetingCandidate* outMetadata
			)
			{
				if (candidate.GetIsPendingDestroy() ||
					(query.excludeSource && query.source == &candidate) ||
					ContainsActor(query.excludedTargets, &candidate))
				{
					return false;
				}

				if (query.requiredTargetLayers != CollisionLayer::None &&
					!HasCollisionLayer(candidate.GetCollisionLayer(), query.requiredTargetLayers))
				{
					return false;
				}

				if (query.requiredCandidateCollisionMask != CollisionLayer::None &&
					!HasCollisionLayer(candidate.GetCollisionMask(), query.requiredCandidateCollisionMask))
				{
					return false;
				}

				if (query.requireCollisionCompatibility && query.source &&
					(!query.source->CanCollideWith(&candidate) ||
						!candidate.CanCollideWith(query.source)))
				{
					return false;
				}

				const sf::Vector2f offset = candidate.GetActorLocation() - query.origin;
				TargetingCandidate metadata;
				metadata.distanceSquared = 0.f;
				metadata.angleRadians = 0.f;
				if (!IsInsideQueryShape(offset, query, metadata.distanceSquared, metadata.angleRadians))
				{
					return false;
				}

				if (query.filter && !query.filter(query.source, candidate, metadata))
				{
					return false;
				}

				if (outMetadata)
				{
					*outMetadata = metadata;
				}
				return true;
			}

			List<TargetingCandidate> FindTargetsInternal(
				const List<weak_ptr<Actor>>& candidates,
				const TargetingQuery& query
			)
			{
				List<TargetingCandidate> result;
				Set<Actor*> seenActors;
				result.reserve(candidates.size());

				for (const weak_ptr<Actor>& candidateWeak : candidates)
				{
					const shared_ptr<Actor> candidate = candidateWeak.lock();
					if (!candidate || !seenActors.insert(candidate.get()).second)
					{
						continue;
					}

					TargetingCandidate metadata;
					if (!EvaluateCandidate(*candidate, query, &metadata))
					{
						continue;
					}

					metadata.actor = candidate;
					result.push_back(std::move(metadata));
				}

				if (query.comparator)
				{
					std::stable_sort(result.begin(), result.end(), query.comparator);
				}
				else
				{
					std::stable_sort(result.begin(), result.end(), [](
						const TargetingCandidate& left,
						const TargetingCandidate& right
					)
					{
						return left.distanceSquared < right.distanceSquared;
					});
				}

				if (query.selector)
				{
					query.selector(result, query.maxTargets);
				}

				if (query.maxTargets > 0 && result.size() > query.maxTargets)
				{
					result.resize(query.maxTargets);
				}
				return result;
			}
		}

		List<TargetingCandidate> AutoTargeting::FindTargets(
			World& world,
			const TargetingQuery& query
		)
		{
			const List<weak_ptr<Actor>> candidates = query.candidateProvider
				? query.candidateProvider(world, query)
				: world.GetActorsInBounds(
					swept::RadiusBounds(query.origin, std::max(0.f, query.range))
				);
			return FindTargetsInternal(candidates, query);
		}

		List<TargetingCandidate> AutoTargeting::FindTargets(
			const List<weak_ptr<Actor>>& candidates,
			const TargetingQuery& query
		)
		{
			return FindTargetsInternal(candidates, query);
		}

		weak_ptr<Actor> AutoTargeting::FindTarget(
			World& world,
			const TargetingQuery& query
		)
		{
			TargetingQuery singleTargetQuery = query;
			singleTargetQuery.maxTargets = 1;
			const List<TargetingCandidate> targets = FindTargets(world, singleTargetQuery);
			return targets.empty() ? weak_ptr<Actor>{} : weak_ptr<Actor>{ targets.front().actor };
		}

		weak_ptr<Actor> AutoTargeting::FindTarget(
			const List<weak_ptr<Actor>>& candidates,
			const TargetingQuery& query
		)
		{
			TargetingQuery singleTargetQuery = query;
			singleTargetQuery.maxTargets = 1;
			const List<TargetingCandidate> targets = FindTargets(candidates, singleTargetQuery);
			return targets.empty() ? weak_ptr<Actor>{} : weak_ptr<Actor>{ targets.front().actor };
		}

		bool AutoTargeting::IsValidTarget(
			const Actor& candidate,
			const TargetingQuery& query
		)
		{
			return EvaluateCandidate(candidate, query, nullptr);
		}

		weak_ptr<Actor> TargetLock::Acquire(World& world, const TargetingQuery& query)
		{
			mTarget = AutoTargeting::FindTarget(world, query);
			return mTarget;
		}

		weak_ptr<Actor> TargetLock::Update(
			World& world,
			const TargetingQuery& query,
			TargetLockMode mode
		)
		{
			const shared_ptr<Actor> currentTarget = mTarget.lock();
			if (mode == TargetLockMode::ReevaluateEveryRefresh)
			{
				return Acquire(world, query);
			}

			if (mode == TargetLockMode::Snapshot)
			{
				if (!currentTarget || currentTarget->GetIsPendingDestroy())
				{
					mTarget.reset();
				}
				return mTarget;
			}

			if (currentTarget && AutoTargeting::IsValidTarget(*currentTarget, query))
			{
				return mTarget;
			}

			mTarget.reset();
			if (mode == TargetLockMode::ReacquireIfInvalid)
			{
				return Acquire(world, query);
			}
			return mTarget;
		}

		void TargetLock::Clear()
		{
			mTarget.reset();
		}

		bool TargetLock::HasTarget() const
		{
			const shared_ptr<Actor> target = mTarget.lock();
			return target && !target->GetIsPendingDestroy();
		}
	}
}
