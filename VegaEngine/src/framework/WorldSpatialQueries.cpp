#include "framework/World.h"
#include "framework/Actor.h"
#include "framework/Application.h"
#include "framework/WorldRenderPipeline.h"
#include "framework/RenderViewport.h"
#include "gameplay/GameStage.h"
#include "widget/HUD.h"
#include "framework/PerfMonitor.h"
#include "framework/PhysicsScene.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <typeinfo>
#include <cstdint>
#include <iterator>
#include <unordered_set>
#include <utility>
#include <vector>

#include "WorldInternal.h"

namespace vega{
	using world_internal::Intersects;
	namespace
	{
		constexpr float ManualSpatialCellSize = 256.f;
	}

	bool World::ShouldUseManualSpatialQuery(const Actor& actor)
	{
		// Actors with a Box2D shape are already returned by the physics
		// broadphase. Keep every body-less actor here to preserve the legacy
		// query contract: tests and gameplay coordinators may intentionally have
		// no collision layer yet still be discoverable by a spatial query.
		return !actor.HasPhysicsBody() &&
			!actor.GetIsPendingDestroy();
	}

	sf::FloatRect World::GetManualSpatialBounds(const Actor& actor)
	{
		const float radius = std::max(0.f, actor.GetPhysicsCollisionRadius());
		const sf::Vector2f location = actor.GetActorLocation();
		sf::Vector2f minimum{ location.x - radius, location.y - radius };
		sf::Vector2f maximum{ location.x + radius, location.y + radius };
		bool hasShape = radius > 0.f;
		// Body-less static geometry still has collision boxes. Index their full
		// bounds, not just the actor origin, so exact edge/end contacts are found.
		const float rotation = actor.GetActorRotation() * 0.01745329251994329577f;
		const float cosine = std::cos(rotation);
		const float sine = std::sin(rotation);
		for (std::size_t index = 0; index < actor.GetPhysicsCollisionBoxCount(); ++index)
		{
			const PhysicsCollisionBox box = actor.GetPhysicsCollisionBox(index);
			if (box.halfExtents.x <= 0.f || box.halfExtents.y <= 0.f) continue;
			const sf::Vector2f center = location + sf::Vector2f{
				cosine * box.localCenter.x - sine * box.localCenter.y,
				sine * box.localCenter.x + cosine * box.localCenter.y };
			const float angle = rotation + box.localRotationDegrees * 0.01745329251994329577f;
			const float c = std::abs(std::cos(angle));
			const float s = std::abs(std::sin(angle));
			const sf::Vector2f extent{ c * box.halfExtents.x + s * box.halfExtents.y,
				s * box.halfExtents.x + c * box.halfExtents.y };
			minimum.x = std::min(minimum.x, center.x - extent.x);
			minimum.y = std::min(minimum.y, center.y - extent.y);
			maximum.x = std::max(maximum.x, center.x + extent.x);
			maximum.y = std::max(maximum.y, center.y + extent.y);
			hasShape = true;
		}
		if (hasShape) return { minimum, maximum - minimum };

		const sf::FloatRect bounds = actor.GetActorGlobalBounds();
		if (bounds.size.x > 0.f || bounds.size.y > 0.f)
		{
			return bounds;
		}

		return { actor.GetActorLocation(), { 0.f, 0.f } };
	}

	World::ManualSpatialCellRange World::GetManualSpatialCellRange(
		const sf::FloatRect& bounds
	)
	{
		const float right = bounds.position.x + std::max(0.f, bounds.size.x);
		const float bottom = bounds.position.y + std::max(0.f, bounds.size.y);
		return {
			static_cast<int>(std::floor(bounds.position.x / ManualSpatialCellSize)),
			static_cast<int>(std::floor(right / ManualSpatialCellSize)),
			static_cast<int>(std::floor(bounds.position.y / ManualSpatialCellSize)),
			static_cast<int>(std::floor(bottom / ManualSpatialCellSize))
		};
	}

	void World::RegisterActorSpatialQuery(Actor& actor)
	{
		mSpatiallyRegisteredActors.insert(&actor);
		RefreshActorSpatialQuery(actor);
	}

	void World::RemoveActorSpatialQuery(Actor& actor)
	{
		const auto rangeIt = mManualSpatialActorCells.find(&actor);
		if (rangeIt != mManualSpatialActorCells.end())
		{
			const ManualSpatialCellRange range = rangeIt->second;
			for (int y = range.minY; y <= range.maxY; ++y)
			{
				for (int x = range.minX; x <= range.maxX; ++x)
				{
					const ManualSpatialCell cell{ x, y };
					auto cellIt = mManualSpatialCells.find(cell);
					if (cellIt == mManualSpatialCells.end())
					{
						continue;
					}

					List<Actor*>& actors = cellIt->second;
					actors.erase(
						std::remove(actors.begin(), actors.end(), &actor),
						actors.end()
					);
					if (actors.empty())
					{
						mManualSpatialCells.erase(cellIt);
					}
				}
			}
			mManualSpatialActorCells.erase(rangeIt);
		}

		mSpatiallyRegisteredActors.erase(&actor);
	}

	void World::RefreshActorSpatialQuery(Actor& actor)
	{
		if (mSpatiallyRegisteredActors.find(&actor) ==
			mSpatiallyRegisteredActors.end())
		{
			return;
		}

		const auto existingRange = mManualSpatialActorCells.find(&actor);
		if (existingRange != mManualSpatialActorCells.end())
		{
			const ManualSpatialCellRange range = existingRange->second;
			for (int y = range.minY; y <= range.maxY; ++y)
			{
				for (int x = range.minX; x <= range.maxX; ++x)
				{
					const ManualSpatialCell cell{ x, y };
					auto cellIt = mManualSpatialCells.find(cell);
					if (cellIt == mManualSpatialCells.end())
					{
						continue;
					}
					List<Actor*>& actors = cellIt->second;
					actors.erase(
						std::remove(actors.begin(), actors.end(), &actor),
						actors.end()
					);
					if (actors.empty())
					{
						mManualSpatialCells.erase(cellIt);
					}
				}
			}
			mManualSpatialActorCells.erase(existingRange);
		}

		if (!ShouldUseManualSpatialQuery(actor))
		{
			return;
		}

		const ManualSpatialCellRange range =
			GetManualSpatialCellRange(GetManualSpatialBounds(actor));
		for (int y = range.minY; y <= range.maxY; ++y)
		{
			for (int x = range.minX; x <= range.maxX; ++x)
			{
				mManualSpatialCells[ManualSpatialCell{ x, y }].push_back(&actor);
			}
		}
		mManualSpatialActorCells.emplace(&actor, range);
	}

	List<weak_ptr<Actor>> World::GetActorsInBounds(
		const sf::FloatRect& bounds
	) const
	{
		List<weak_ptr<Actor>> result;
		ForEachActorInBounds(bounds, [&result](Actor& actor)
		{
			const shared_ptr<Object> object = actor.GetWeakPtr().lock();
			if (object)
			{
				result.push_back(std::static_pointer_cast<Actor>(object));
			}
		});
		return result;
	}

	void World::VisitActorsInBounds(
		const sf::FloatRect& bounds,
		void* context,
		ActorBoundsVisitor visitor
	) const
	{
		if (!visitor)
		{
			return;
		}

		struct PhysicsVisitContext
		{
			void* visitorContext = nullptr;
			ActorBoundsVisitor visitor = nullptr;
			std::unordered_set<std::uint64_t>* visitedActorIds = nullptr;
			bool stopped = false;
		};
		std::unordered_set<std::uint64_t> visitedActorIds;
		PhysicsVisitContext physicsContext{ context, visitor, &visitedActorIds, false };
		mPhysicsScene->VisitActorsInBounds(
			bounds,
			&physicsContext,
			[](void* rawContext, Actor* actor)
			{
				auto* visit = static_cast<PhysicsVisitContext*>(rawContext);
				if (!visit || !actor || actor->GetIsPendingDestroy() ||
					actor->GetWeakPtr().expired())
				{
					return true;
				}
				if (!visit->visitedActorIds->insert(actor->GetUniqueID()).second)
				{
					return true;
				}
				visit->stopped = !visit->visitor(visit->visitorContext, actor);
				return !visit->stopped;
			}
		);
		if (physicsContext.stopped)
		{
			return;
		}

		// Swept projectiles deliberately avoid Box2D bodies, but they still need
		// spatial queries. Traverse only the matching cells of their dedicated
		// index instead of scanning every actor in the world.
		const ManualSpatialCellRange range = GetManualSpatialCellRange(bounds);
		std::uint64_t queryStamp = ++mManualSpatialQueryStamp;
		if (queryStamp == 0)
		{
			// Wraparound is practically unreachable, but zero is reserved as the
			// initial per-actor stamp so retain a valid marker if it happens.
			queryStamp = ++mManualSpatialQueryStamp;
		}

		// Collect first: the visitor may destroy or move actors, which mutates the
		// cell containers being traversed here.
		std::vector<Actor*> candidates;
		for (int y = range.minY; y <= range.maxY; ++y)
		{
			for (int x = range.minX; x <= range.maxX; ++x)
			{
				const auto cellIt = mManualSpatialCells.find(ManualSpatialCell{ x, y });
				if (cellIt == mManualSpatialCells.end())
				{
					continue;
				}

				for (Actor* actor : cellIt->second)
				{
					if (!actor || actor->GetWorld() != this ||
						actor->GetIsPendingDestroy() ||
						actor->mLastManualSpatialQueryStamp == queryStamp)
					{
						continue;
					}
					actor->mLastManualSpatialQueryStamp = queryStamp;
					if (!Intersects(GetManualSpatialBounds(*actor), bounds))
					{
						continue;
					}
					if (!visitedActorIds.insert(actor->GetUniqueID()).second)
					{
						continue;
					}
					candidates.push_back(actor);
				}
			}
		}

		for (Actor* actor : candidates)
		{
			if (actor->GetWorld() != this || actor->GetIsPendingDestroy())
			{
				continue;
			}
			if (!visitor(context, actor))
			{
				return;
			}
		}
	}
	weak_ptr<Actor> World::GetActorByLayer(CollisionLayer layer) const
	{
		for (auto& actor : mActors)
		{
			if (!actor->GetIsPendingDestroy() && actor->GetCollisionLayer() == layer)
			{
				return actor;
			}
		}
		return weak_ptr<Actor>();
	}

}
