#pragma once

#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "presentation/ability/cryoBola/CryoBolaPresentationProfile.h"

namespace ly
{
	// A physical, single-hit skill-shot. Its rupture exists only after a valid
	// combatant hit, making the struck enemy the AoE anchor rather than a point
	// selected freely on the ground.
	class CryoBolaProjectileActor final : public AbilityWorldActor
	{
	public:
		CryoBolaProjectileActor(
			World* world,
			Actor* owner,
			const CryoBolaPresentationProfile& presentationProfile
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

	private:
		void Move(float deltaTime);
		void ApplySweptHit(
			const sf::Vector2f& startLocation,
			const sf::Vector2f& endLocation
		);
		void ResolveHit(Actor& primaryTarget);
		void ResolveMiss();
		void ApplyRupture(Actor& primaryTarget);
		void DrawProjectile(sf::RenderWindow& window) const;
		void DrawRupture(sf::RenderWindow& window) const;

		CryoBolaPresentationProfile mPresentationProfile;
		float mProjectileSpeed = 0.f;
		float mMaximumRange = 0.f;
		float mRuptureRadius = 0.f;
		float mTravelDistance = 0.f;
		float mRuptureAge = 0.f;
		sf::Vector2f mLaunchVelocity{};
		bool mResolved = false;
		bool mHitResolved = false;
	};

	bool RegisterCryoBolaProjectileActorType();
}
