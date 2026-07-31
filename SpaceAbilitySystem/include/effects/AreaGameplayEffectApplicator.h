#pragma once

#include "effects/GameplayEffectHandle.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace sas
{
	template <typename Target, typename RuntimeContext>
	class AreaGameplayEffectApplicator
	{
	public:
		using TargetList = std::vector<std::shared_ptr<Target>>;
		using ContextPtr = std::shared_ptr<RuntimeContext>;
		using ContextFactory = std::function<ContextPtr(Target&)>;
		using ContextCallback = std::function<void(RuntimeContext&)>;
		using IsActiveCallback =
			std::function<bool(Target&, GameplayEffectHandle)>;
		using RefreshCallback =
			std::function<void(Target&, GameplayEffectHandle)>;
		using ApplyCallback =
			std::function<GameplayEffectHandle(Target&, ContextPtr)>;

		void Update(
			const TargetList& targetsInside,
			const IsActiveCallback& isActive,
			const RefreshCallback& refresh,
			const ApplyCallback& apply,
			const ContextFactory& createContext = {},
			const ContextCallback& onStay = {},
			const ContextCallback& onExit = {}
		)
		{
			for (std::size_t index = 0; index < mTrackedEffects.size();)
			{
				TrackedEffect& tracked = mTrackedEffects[index];
				const std::shared_ptr<Target> target = tracked.target.lock();
				const bool stillInside = target && std::any_of(
					targetsInside.begin(),
					targetsInside.end(),
					[&target](const std::shared_ptr<Target>& candidate)
					{
						return candidate.get() == target.get();
					}
				);
				const bool effectStillActive =
					target && isActive && isActive(*target, tracked.handle);
				if (stillInside && effectStillActive)
				{
					if (tracked.runtimeContext && onStay)
					{
						onStay(*tracked.runtimeContext);
					}
					if (refresh)
					{
						refresh(*target, tracked.handle);
					}
					++index;
					continue;
				}

				if (tracked.runtimeContext && onExit)
				{
					onExit(*tracked.runtimeContext);
				}
				mTrackedEffects.erase(
					mTrackedEffects.begin() + static_cast<std::ptrdiff_t>(index)
				);
			}

			for (const std::shared_ptr<Target>& target : targetsInside)
			{
				if (!target)
				{
					continue;
				}
				const bool alreadyTracked = std::any_of(
					mTrackedEffects.begin(),
					mTrackedEffects.end(),
					[&target](const TrackedEffect& tracked)
					{
						const std::shared_ptr<Target> existing = tracked.target.lock();
						return existing && existing.get() == target.get();
					}
				);
				if (alreadyTracked || !apply)
				{
					continue;
				}

				ContextPtr runtimeContext =
					createContext ? createContext(*target) : ContextPtr{};
				const GameplayEffectHandle handle =
					apply(*target, runtimeContext);
				if (handle.IsValid())
				{
					mTrackedEffects.push_back(TrackedEffect{
						target,
						handle,
						std::move(runtimeContext)
					});
				}
			}
		}

		void Clear(const ContextCallback& onExit = {})
		{
			for (TrackedEffect& tracked : mTrackedEffects)
			{
				if (tracked.runtimeContext && onExit)
				{
					onExit(*tracked.runtimeContext);
				}
			}
			mTrackedEffects.clear();
		}

		std::size_t GetTrackedTargetCount() const
		{
			return mTrackedEffects.size();
		}

	private:
		struct TrackedEffect
		{
			std::weak_ptr<Target> target;
			GameplayEffectHandle handle;
			ContextPtr runtimeContext;
		};

		std::vector<TrackedEffect> mTrackedEffects;
	};
}
