#include "framework/ActorRef.h"
#include "framework/Actor.h"
#include "framework/World.h"
#include "framework/scene/SceneEntityComponent.h"

namespace ly
{
	ActorRef::ActorRef(Actor& actor)
		: mGuid{ actor.GetOrAddComponent<SceneEntityComponent>().guid }, mActor{ actor.GetWeakActor() }
	{
	}

	std::shared_ptr<Actor> ActorRef::Resolve(World& world) const
	{
		if (mGuid.empty()) return nullptr;
		if (auto actor = mActor.lock(); actor && actor->GetWorld() == &world && !actor->GetIsPendingDestroy())
			if (const auto* identity = actor->FindComponent<SceneEntityComponent>(); identity && identity->guid == mGuid) return actor;
		// Cache failed lookups too: a dangling authoring reference must not scan each frame.
		const auto revision = world.ActorReferenceRevision();
		if (mWorld == world.GetUniqueID() && mRevision == revision) return nullptr;
		mWorld = world.GetUniqueID();
		mRevision = revision;
		for (const auto& weak : world.GetActorsByTypeIncludingPending<Actor>())
			if (auto actor = weak.lock(); actor && !actor->GetIsPendingDestroy())
				if (const auto* identity = actor->FindComponent<SceneEntityComponent>(); identity && identity->guid == mGuid)
				{
					mActor = actor;
					return actor;
				}
		return nullptr;
	}
}
