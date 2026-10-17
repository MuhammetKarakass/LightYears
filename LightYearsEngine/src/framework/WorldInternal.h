#pragma once

#include "framework/Actor.h"
#include <chrono>
#include <typeinfo>

namespace ly::world_internal
{
		inline bool Intersects(const sf::FloatRect& first, const sf::FloatRect& second)
		{
			return first.position.x + first.size.x >= second.position.x &&
				first.position.x <= second.position.x + second.size.x &&
				first.position.y + first.size.y >= second.position.y &&
				first.position.y <= second.position.y + second.size.y;
		}

		// Hitch tracing: an actor whose tick, BeginPlay or render takes a millisecond or more is reported under its
		// class name, so a slow frame names the actor type responsible.
		class SlowActorTrace
		{
		public:
			explicit SlowActorTrace(const Actor& actor)
#if LY_ENABLE_PROFILING
				: mActor(actor), mStart(std::chrono::steady_clock::now())
#endif
			{
				(void)actor;
			}

			~SlowActorTrace()
			{
#if LY_ENABLE_PROFILING
				const double elapsed = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - mStart).count();
				if (elapsed >= debug::Profiler::FrameEventThresholdMicroseconds)
				{
					debug::Profiler::RecordFrameEvent(typeid(mActor).name(), elapsed);
				}
#endif
			}

		private:
#if LY_ENABLE_PROFILING
			const Actor& mActor;
			std::chrono::steady_clock::time_point mStart;
#endif
		};
}
