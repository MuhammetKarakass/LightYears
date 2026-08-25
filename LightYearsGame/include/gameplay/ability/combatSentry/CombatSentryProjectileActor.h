#pragma once

#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "presentation/ability/combatSentry/CombatSentryPresentationProfile.h"

namespace ly
{
	class CombatSentryProjectileActor final : public AbilityWorldActor
	{
	public:
		CombatSentryProjectileActor(
			World* world,
			Actor* owner,
			const CombatSentryProjectilePresentationProfile& profile
		);

		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;
		void OnActorBeginOverlap(Actor* otherActor) override;
		void ConfigureFromAttributes(
			const sas::GameplayAttributeList& attributes
		) override;
		bool IsProjectileActor() const override { return true; }
		bool CanBeReflected() const override { return true; }
		bool TryReflectProjectile(const ProjectileReflectionRequest& request) override;
		void SetShotDamage(float damage) { SetDamage(damage); }

	private:
		void Move(float deltaTime);

		CombatSentryProjectilePresentationProfile mProfile;
		float mSpeed = 1300.f;
		float mMaximumRange = 800.f;
		float mTravelDistance = 0.f;
	};

	bool RegisterCombatSentryProjectileActorType();
}
