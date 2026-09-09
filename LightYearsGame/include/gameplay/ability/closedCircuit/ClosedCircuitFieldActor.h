#pragma once

#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameplay/projectile/ProjectileInterceptionBoundary.h"
#include "presentation/ability/closedCircuit/ClosedCircuitPresentationProfile.h"

namespace ly
{
	// One actor owns the complete delivery lifecycle: it travels as a visible
	// projectile, forms at its destination, then becomes the interception field.
	class ClosedCircuitFieldActor final : public AbilityWorldActor, public ProjectileInterceptionBoundary
	{
	public:
		ClosedCircuitFieldActor(World* world, Actor* owner, const ClosedCircuitPresentationProfile& profile);
		void ConfigureDelivery(const sf::Vector2f& target, float speed, float formationDuration, float barrierRadius, float barrierHealth);
		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;
		void Destroy() override;
		bool TryInterceptProjectile(AbilityWorldActor& projectile, const sf::Vector2f& previousLocation) override;

	private:
		enum class Phase { Delivery, Forming, Active };
		void ActivateField();

		ClosedCircuitPresentationProfile mProfile;
		Phase mPhase = Phase::Delivery;
		sf::Vector2f mTarget{};
		float mDeliverySpeed = 280.f;
		float mFormationDuration = 0.50f;
		float mPhaseElapsed = 0.f;
		float mBarrierRadius = 250.f;
		float mRemainingHealth = 0.f;
		float mMaximumHealth = 0.f;
	};
	bool RegisterClosedCircuitFieldActorType();
}
