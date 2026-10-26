#pragma once
#include "framework/Actor.h"
#include "presentation/ability/ionComet/IonCometPresentationProfile.h"

namespace vega
{
	// Presentation only. The module runtime owns damage, charge and the visual's lifetime.
	class IonCometVisualActor final : public Actor
	{
	public:
		IonCometVisualActor(World* world, weak_ptr<Actor> owner, const IonCometPresentationProfile& profile);
		void Tick(float deltaSeconds) override;
		void Render(sf::RenderTarget& target) override;
		void SetFieldRadius(float radius) { mFieldRadius = radius; }
		float GetFieldRadius() const { return mFieldRadius; }
		void Burst(const sf::Vector2f& origin, float radius);
	private:
		weak_ptr<Actor> mOwner;
		IonCometPresentationProfile mProfile;
		float mFieldRadius = 0.f;
		float mBurstRadius = 0.f;
		float mBurstRemaining = 0.f;
		sf::Vector2f mBurstOrigin{};
	};
}
