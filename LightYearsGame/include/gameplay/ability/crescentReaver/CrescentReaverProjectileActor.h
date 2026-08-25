#pragma once

#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "presentation/ability/crescentReaver/CrescentReaverPresentationProfile.h"

#include <cstddef>
#include <optional>

namespace ly
{
	class CrescentReaverProjectileActor final : public AbilityWorldActor
	{
	public:
		CrescentReaverProjectileActor(
			World* world,
			Actor* owner,
			const CrescentReaverPresentationProfile& presentationProfile
		);

		void BeginPlay() override;
		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;
		void OnActorBeginOverlap(Actor* otherActor) override;
		void ConfigureFromAttributes(
			const sas::GameplayAttributeList& attributes
		) override;
		bool IsProjectileActor() const override { return true; }
		weak_ptr<AbilityWorldActor> SpawnRelayClone(
			const ProjectileRelayCloneRequest& request
		) const override;

		float GetProjectileSpeed() const { return mProjectileSpeed; }
		int GetBounceCountRemaining() const { return mBounceCountRemaining; }
		int GetCompletedBounceCount() const { return mCompletedBounceCount; }
		bool IsDissipating() const { return mIsDissipating; }

	private:
		struct ImpactPulse
		{
			sf::Vector2f location{};
			float age = 0.f;
		};

		struct SweepHit
		{
			Actor* actor = nullptr;
			float normalizedTime = 0.f;
			sf::Vector2f normal{};
			bool isSurface = false;
		};

		void Move(float deltaTime);
		bool AdvanceWithBounce(float deltaTime);
		std::optional<SweepHit> FindNearestCollision(
			const sf::Vector2f& startLocation,
			const sf::Vector2f& endLocation
		) const;
		bool TryProcessCollision(const SweepHit& hit);
		void ReduceSourceAbilityCooldown();
		void BeginDissipation();
		void DrawCrescent(
			sf::RenderWindow& window,
			float opacity = 1.f,
			float scale = 1.f
		) const;
		void DrawDissipation(sf::RenderWindow& window) const;
		void DrawImpacts(sf::RenderWindow& window) const;

		CrescentReaverPresentationProfile mPresentationProfile;
		float mProjectileSpeed = 0.f;
		float mInitialDamage = 0.f;
		int mBounceCountRemaining = 0;
		int mCompletedBounceCount = 0;
		float mBounceDamageGrowth = 0.f;
		float mBounceCooldownReduction = 0.f;
		float mSpinDegrees = 0.f;
		float mDissipationAge = 0.f;
		sf::Vector2f mLaunchVelocity{};
		Actor* mIgnoredCollisionActor = nullptr;
		List<ImpactPulse> mImpactPulses;
		bool mIsDissipating = false;
	};

	bool RegisterCrescentReaverProjectileActorType();
}
