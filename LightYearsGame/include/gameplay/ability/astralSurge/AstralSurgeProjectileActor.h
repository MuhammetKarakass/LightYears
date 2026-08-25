#pragma once

#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "presentation/ability/astralSurge/AstralSurgePresentationProfile.h"

#include <cstddef>

namespace ly
{
	class AstralSurgeProjectileActor final : public AbilityWorldActor
	{
	public:
		AstralSurgeProjectileActor(
			World* world,
			Actor* owner,
			const AstralSurgePresentationProfile& presentationProfile
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
		float GetProjectileWidth() const { return mProjectileWidth; }
		float GetPierceDamageLoss() const { return mPierceDamageLoss; }
		float GetMinimumDamageMultiplier() const { return mMinimumDamageMultiplier; }
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
		bool IsOutsideTechnicalBounds() const;

		AstralSurgePresentationProfile mPresentationProfile;
		float mProjectileSpeed = 0.f;
		float mProjectileWidth = 0.f;
		float mPierceDamageLoss = 0.f;
		float mMinimumDamageMultiplier = 0.40f;
		sf::Vector2f mLaunchVelocity{};
		Set<Actor*> mHitTargets;
		List<ImpactPulse> mImpactPulses;
	};

	bool RegisterAstralSurgeProjectileActorType();
}
