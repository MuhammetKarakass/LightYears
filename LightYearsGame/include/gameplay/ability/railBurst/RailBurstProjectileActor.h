#pragma once

#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "presentation/ability/railBurst/RailBurstPresentationProfile.h"

#include <cstddef>

namespace ly
{
	class RailBurstProjectileActor final : public AbilityWorldActor
	{
	public:
		RailBurstProjectileActor(
			World* world,
			Actor* owner,
			const RailBurstPresentationProfile& presentationProfile
		);

		void BeginPlay() override;
		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;
		void OnActorBeginOverlap(Actor* otherActor) override;
		void ConfigureFromAttributes(
			const sas::GameplayAttributeList& attributes
		) override;
		bool IsProjectileActor() const override { return true; }
		bool CanBeReflected() const override { return true; }
		bool TryReflectProjectile(const ProjectileReflectionRequest& request) override;
		weak_ptr<AbilityWorldActor> SpawnRelayClone(
			const ProjectileRelayCloneRequest& request
		) const override;

		float GetProjectileSpeed() const { return mProjectileSpeed; }
		float GetMaximumRange() const { return mMaximumRange; }
		std::size_t GetHitTargetCount() const { return mHitTargets.size(); }

	private:
		struct ImpactPulse
		{
			sf::Vector2f location{};
			float age = 0.f;
		};

		void Move(float deltaTime);
		void ApplySweptHits(
			const sf::Vector2f& startLocation,
			const sf::Vector2f& endLocation
		);
		bool TryHitTarget(Actor* otherActor);
		void ApplyPiercingHit(Actor& target);
		void DrawFlight(sf::RenderWindow& window) const;
		void DrawImpacts(sf::RenderWindow& window) const;

		RailBurstPresentationProfile mPresentationProfile;
		float mProjectileSpeed = 0.f;
		float mMaximumRange = 0.f;
		float mTravelDistance = 0.f;
		sf::Vector2f mLaunchVelocity{};
		Set<Actor*> mHitTargets;
		List<ImpactPulse> mImpactPulses;
	};

	bool RegisterRailBurstProjectileActorType();
}
