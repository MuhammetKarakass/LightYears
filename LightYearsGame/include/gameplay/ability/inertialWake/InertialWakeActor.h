#pragma once

#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "presentation/ability/inertialWake/InertialWakePresentationProfile.h"

#include <unordered_map>

namespace ly
{
	class SpaceShip;

	class InertialWakeActor final : public AbilityWorldActor
	{
	public:
		InertialWakeActor(
			World* world,
			Actor* owner,
			const InertialWakePresentationProfile& presentationProfile
		);

		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;
		void ConfigureFromAttributes(const sas::GameplayAttributeList& attributes) override;

	private:
		float ResolveEffectiveSpeedRatio(float speed, float normalTopSpeed) const;
		bool IsInsideWakeEdge(
			const sf::Vector2f& point,
			float length,
			float edgeThickness,
			float targetRadius
		) const;
		void ApplyWakeHit(Actor& target, float speed, float effectiveRatio);
		void ApplyStun(SpaceShip& target, float duration) const;
		void UpdateGeometry();

		InertialWakePresentationProfile mPresentationProfile;
		std::unordered_map<const Actor*, float> mNextHitTime;
		float mSpeedDamageConversion = 0.30f;
		float mEnergyMaxReference = 50.f;
		float mEnergyMaxConversionPerPoint = 0.0001f;
		float mSameTargetHitCooldown = 2.f;
		float mMinimumSpeedRatio = 0.20f;
		float mBaseEdgeThickness = 3.f;
		float mLengthPerEffectiveRatio = 176.f;
		float mWidthPerEffectiveRatio = 1.5f;
		float mOpeningAngleDegrees = 130.f;
		float mDiminishingStartRatio = 1.5f;
		float mDiminishingExcessMultiplier = 0.5f;
		float mBaseKnockbackSpeed = 120.f;
		float mKnockbackPerEffectiveRatio = 140.f;
		float mBaseStunDuration = 0.25f;
		float mStunPerEffectiveRatio = 0.20f;
		float mCurrentLength = 0.f;
		float mCurrentWidth = 0.f;
		float mElapsed = 0.f;
	};

	bool RegisterInertialWakeActorType();
}
