#include "framework/EngineContext.h"
#include "framework/ActorComponent.h"
#include "presentation/ability/common/PresentationColor.h"
#include "presentation/ability/common/TimedPresentationActor.h"

#include <cmath>
#include <iostream>

namespace
{
	bool Expect(bool condition, const char* message)
	{
		if (!condition)
		{
			std::cerr << "FAILED: " << message << '\n';
			return false;
		}
		return true;
	}

	class TickProbeComponent final : public vega::ActorComponent
	{
	public:
		explicit TickProbeComponent(vega::Actor& owner) : ActorComponent(owner) { mCanEverTick = true; }
		void TickComponent(float) override { ++ticks; }

		int ticks = 0;
	};

	struct Probe
	{
		int hookCalls = 0;
		float ageAtHook = -1.f;
		int componentTicksAtHook = -1;
		TickProbeComponent* component = nullptr;
	};

	class ProbeTimedActor final : public vega::TimedPresentationActor
	{
	public:
		ProbeTimedActor(vega::World* world, const sf::Vector2f& worldLocation, float duration, vega::RenderLayer renderLayer, Probe& probe)
			: TimedPresentationActor(world, worldLocation, duration, renderLayer, world ? nullptr : &vega::EngineContext::Current()), mProbe(probe)
		{
			mProbe.component = &AddComponent<TickProbeComponent>();
		}

		float Age() const { return GetAge(); }
		float Duration() const { return GetDuration(); }

	private:
		void UpdateVisuals() override
		{
			++mProbe.hookCalls;
			mProbe.ageAtHook = GetAge();
			mProbe.componentTicksAtHook = mProbe.component ? mProbe.component->ticks : -1;
		}

		Probe& mProbe;
	};
}

int main()
{
	// Setup below runs before any Application exists; applications adopt this binding.
	vega::EngineContext testContext;
	vega::EngineContext::Scope testScope{ testContext };
	const sf::Color halfAlpha = vega::WithAlpha(sf::Color{ 10, 20, 30, 200 }, 0.5f);
	const sf::Color overAlpha = vega::WithAlpha(sf::Color{ 10, 20, 30, 200 }, 2.f);
	const sf::Color zeroAlpha = vega::WithAlpha(sf::Color{ 10, 20, 30, 200 }, -1.f);
	if (!Expect(vega::Saturate(-1.f) == 0.f && vega::Saturate(0.5f) == 0.5f && vega::Saturate(2.f) == 1.f, "Saturate did not clamp to [0, 1]") ||
		!Expect(halfAlpha.r == 10 && halfAlpha.g == 20 && halfAlpha.b == 30 && halfAlpha.a == 100, "WithAlpha scaled the wrong channel") ||
		!Expect(overAlpha.a == 200, "WithAlpha did not clamp a bright multiplier to the source alpha") ||
		!Expect(zeroAlpha.a == 0, "WithAlpha did not clamp a negative multiplier"))
	{
		return 1;
	}

	Probe probe;
	ProbeTimedActor actor{ nullptr, sf::Vector2f{ 12.f, 34.f }, 0.5f, vega::RenderLayer::GroundDecal, probe };
	if (!Expect(actor.GetActorLocation().x == 12.f && actor.GetActorLocation().y == 34.f, "Constructor did not apply the world location") ||
		!Expect(actor.GetRenderLayer() == vega::RenderLayer::GroundDecal, "Constructor did not apply the render layer") ||
		!Expect(actor.GetCollisionLayer() == vega::CollisionLayer::None, "Constructor did not clear the collision layer") ||
		!Expect(actor.GetCollisionMask() == vega::CollisionLayer::None, "Constructor did not clear the collision mask") ||
		!Expect(!actor.IsPhysicsEnabled(), "Constructor did not disable physics") ||
		!Expect(actor.Age() == 0.f, "Age did not start at zero") ||
		!Expect(actor.Duration() == 0.5f, "Duration was not stored"))
	{
		return 1;
	}

	actor.BeginPlayInternal();
	actor.Tick(0.1f);
	if (!Expect(!actor.GetIsPendingDestroy(), "Actor was destroyed before its duration") ||
		!Expect(probe.hookCalls == 1, "UpdateVisuals was not called on a live tick") ||
		!Expect(std::abs(probe.ageAtHook - 0.1f) < 0.0001f, "UpdateVisuals did not see the advanced age") ||
		!Expect(probe.componentTicksAtHook == 0, "UpdateVisuals ran after Actor::Tick instead of before it") ||
		!Expect(probe.component->ticks == 1, "Actor::Tick did not run after UpdateVisuals"))
	{
		return 1;
	}

	actor.Tick(0.4f);
	if (!Expect(actor.GetIsPendingDestroy(), "Actor was not destroyed at its duration") ||
		!Expect(probe.hookCalls == 1, "UpdateVisuals ran on the destruction tick") ||
		!Expect(probe.component->ticks == 1, "Actor::Tick ran on the destruction tick"))
	{
		return 1;
	}

	Probe negativeProbe;
	ProbeTimedActor negativeActor{ nullptr, sf::Vector2f{}, 1.f, vega::RenderLayer::WorldVfx, negativeProbe };
	negativeActor.BeginPlayInternal();
	negativeActor.Tick(-3.f);
	if (!Expect(!negativeActor.GetIsPendingDestroy(), "A negative delta destroyed the actor") ||
		!Expect(negativeActor.Age() == 0.f, "A negative delta moved the age backwards") ||
		!Expect(negativeProbe.hookCalls == 1, "A negative delta skipped the visual update"))
	{
		return 1;
	}

	Probe zeroProbe;
	ProbeTimedActor zeroActor{ nullptr, sf::Vector2f{}, 0.f, vega::RenderLayer::WorldVfx, zeroProbe };
	zeroActor.BeginPlayInternal();
	zeroActor.Tick(0.01f);
	if (!Expect(zeroActor.GetIsPendingDestroy(), "A zero-duration actor survived its first tick") ||
		!Expect(zeroProbe.hookCalls == 0, "A zero-duration actor updated visuals before destruction"))
	{
		return 1;
	}

	return 0;
}
