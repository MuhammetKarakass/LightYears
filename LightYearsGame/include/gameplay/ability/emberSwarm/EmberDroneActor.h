#pragma once

#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "presentation/ability/emberSwarm/EmberSwarmPresentationProfile.h"

#include <SFML/Graphics/CircleShape.hpp>
#include <cstddef>
#include <cstdint>

namespace ly
{
	class EmberDroneActor final : public AbilityWorldActor
	{
	public:
		enum class State : std::uint8_t
		{
			IdleOrbitOwner,
			TravelingToTarget,
			OrbitingTarget
		};

		struct Configuration
		{
			std::size_t droneIndex = 0;
			float orbitRadius = 80.f;
			float targetOrbitRadius = 65.f;
			float angularSpeed = 2.0f;
			float travelSpeed = 1100.f;
			float pulseInterval = 0.25f;
			float pulsePhaseOffset = 0.f;
		};

		EmberDroneActor(
			World* world,
			Actor* owner,
			const EmberSwarmPresentationProfile& presentationProfile,
			const Configuration& configuration = {}
		);

		void BeginPlay() override;
		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;

		// Drone is a summon, not a colliding actor nor projectile.
		bool IsProjectileActor() const override { return false; }

		const Configuration& GetConfiguration() const { return mConfiguration; }
		float GetOrbitAngleRadians() const { return mOrbitAngleRadians; }

		void SetTarget(const shared_ptr<Actor>& target);
		void ClearTarget();
		Actor* GetTarget() const;
		State GetState() const { return mState; }
		bool IsOrbiting() const { return mState == State::OrbitingTarget; }
		bool IsTraveling() const { return mState == State::TravelingToTarget; }
		bool IsIdle() const { return mState == State::IdleOrbitOwner; }

		void SetAbilityLevel(int level) { mAbilityLevel = std::max(1, level); }
		int GetAbilityLevel() const { return mAbilityLevel; }

	private:
		void PerformPulse(Actor& owner, Actor& target);
		float ResolveExpiryFade() const;
		void UpdatePrimitiveGeometry();

		EmberSwarmPresentationProfile mPresentationProfile;
		Configuration mConfiguration;
		State mState = State::IdleOrbitOwner;
		weak_ptr<Actor> mTarget;

		float mOrbitAngleRadians = 0.f;
		float mNextPulseTime = 0.f;
		int mAbilityLevel = 1;

		sf::CircleShape mGlow;
		sf::CircleShape mBody;
		sf::CircleShape mCore;

		// Pulse / impact presentation feedback
		sf::Vector2f mPulseOrigin{};
		sf::Vector2f mImpactLocation{};
		float mPulseVisualTimer = 0.f;
		bool mHasActivePulseVisual = false;
		bool mHasImpactVisual = false;
	};
}

