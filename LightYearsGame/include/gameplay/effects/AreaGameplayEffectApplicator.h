#pragma once

#include "gameplay/effects/ActiveGameplayEffect.h"

#include <functional>

namespace ly
{
	class Actor;

	// Shape/target discovery stays with the owning actor. This reusable layer
	// owns effect handles, duration refresh, exit grace, and source isolation.
	class AreaGameplayEffectApplicator
	{
	public:
		using RuntimeContextFactory =
			std::function<std::shared_ptr<GameplayEffectRuntimeContext>(Actor& target)>;
		using RuntimeContextCallback =
			std::function<void(GameplayEffectRuntimeContext& context)>;

		void Update(
			const List<shared_ptr<Actor>>& targetsInside,
			const GameplayEffectSpec& spec,
			Actor& source,
			const void* sourceScope,
			const RuntimeContextFactory& createRuntimeContext = {},
			const RuntimeContextCallback& onStay = {},
			const RuntimeContextCallback& onExit = {}
		);
		void Clear(const RuntimeContextCallback& onExit = {});

		size_t GetTrackedTargetCount() const { return mTrackedEffects.size(); }

	private:
		struct TrackedEffect
		{
			weak_ptr<Actor> target;
			GameplayEffectHandle handle;
			std::shared_ptr<GameplayEffectRuntimeContext> runtimeContext;
		};

		List<TrackedEffect> mTrackedEffects;
	};
}
