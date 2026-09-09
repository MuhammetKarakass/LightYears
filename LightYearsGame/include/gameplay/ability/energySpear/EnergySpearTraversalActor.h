#pragma once

#include "framework/Actor.h"
#include "gameplay/combat/ContactDamageGuardRegistry.h"
#include "gameplay/damage/DamageContext.h"
#include "presentation/ability/energySpear/EnergySpearPresentationProfile.h"

#include <SFML/Graphics/CircleShape.hpp>

namespace ly
{
	struct EnergySpearTraversalRequest
	{
		Actor* owner = nullptr;
		sf::Vector2f startLocation{};
		sf::Vector2f direction{ 0.f, -1.f };
		float travelDistance = 0.f;
		float travelSpeed = 1.f;
		float collisionRadius = 1.f;
		float minimumDamageDistance = 0.f;
		float endpointDamageMultiplier = 1.f;
		float chargeDamageMultiplier = 1.f;
		float damage = 0.f;
		List<GameplayTag> damageTags;
		DamagePayload damagePayload;
		sas::ContentId sourceAbilityId;
		List<GameplayTag> sourceAbilityTags;
		EnergySpearTraversalVisualDefinition visual;
	};

	// The ability instance ends when the hold input is released. This actor owns
	// only the short traversal phase, so movement, piercing and lock cleanup do
	// not depend on an already-ended WhileInputHeld ability instance.
	class EnergySpearTraversalActor final : public Actor
	{
	public:
		EnergySpearTraversalActor(
			World* world,
			const EnergySpearTraversalRequest& request
		);

		void BeginPlay() override;
		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;
		void Destroy() override;

		int GetHitTargetCount() const
		{
			return static_cast<int>(mHitTargets.size());
		}


	private:
		struct ImpactPulse
		{
			sf::Vector2f location{};
			float age = 0.f;
		};

		void Move(float deltaTime);
		void ApplyPiercingHits(
			const sf::Vector2f& segmentStart,
			const sf::Vector2f& segmentEnd
		);
		void Finish();
		void RegisterContactDamageGuard();
		void UnregisterContactDamageGuard();
		void DrawImpacts(sf::RenderWindow& window) const;

		weak_ptr<Actor> mOwner;
		EnergySpearTraversalVisualDefinition mVisual;
		sf::Vector2f mStartLocation{};
		sf::Vector2f mDirection{ 0.f, -1.f };
		float mTravelDistance = 0.f;
		float mTravelSpeed = 1.f;
		float mCollisionRadius = 1.f;
		float mMinimumDamageDistance = 0.f;
		float mEndpointDamageMultiplier = 1.f;
		float mChargeDamageMultiplier = 1.f;
		float mDamage = 0.f;
		List<GameplayTag> mDamageTags;
		DamagePayload mDamagePayload;
		sas::ContentId mSourceAbilityId;
		List<GameplayTag> mSourceAbilityTags;
		ContactDamageGuardHandle mContactDamageGuardHandle;
		float mDistanceTravelled = 0.f;
		sf::Vector2f mPreservedVelocity{};
		Set<Actor*> mHitTargets;
		List<ImpactPulse> mImpactPulses;
		bool mFinished = false;
	};
}
