#pragma once

#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameplay/movement/MovementInfluenceTypes.h"
#include "presentation/ability/lanceDrive/LanceDrivePresentationProfile.h"

#include <unordered_map>
#include <cstdint>

namespace ly
{
	class LanceDriveActor final : public AbilityWorldActor
	{
	public:
		LanceDriveActor(
			World* world,
			Actor* owner,
			const LanceDrivePresentationProfile& presentationProfile
		);

		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;
		void Destroy() override;
		void ConfigureFromAttributes(const sas::GameplayAttributeList& attributes) override;
		std::size_t GetPhysicsCollisionBoxCount() const override;
		PhysicsCollisionBox GetPhysicsCollisionBox(std::size_t index) const override;

		bool ConsumesFrontalContact(const Actor& source, const Actor& target) const;

	private:
		bool ResolveWallContact(Actor& target, float radius, const sf::Vector2f& previousPosition, float previousRotation, float deltaTime);
		bool IsInsideEdge(const sf::Vector2f& point, float targetRadius) const;
		void ApplyLanceHit(Actor& target, float speed);
		void UpdateGeometry();
		movement::MovementInfluenceSourceId GetMovementSourceId() const;
		void ResolveEdgeEndpoints(
			sf::Vector2f& tip,
			sf::Vector2f& leftEnd,
			sf::Vector2f& rightEnd
		) const;

		LanceDrivePresentationProfile mPresentationProfile;
		std::unordered_map<const Actor*, float> mNextHitTime;
		float mBaseDamage = 10.f;
		float mSpeedDamageConversion = 0.20f;
		float mEnergyPowerReference = 50.f;
		float mEnergyPowerConversionPerPoint = 0.0001f;
		float mSameTargetHitCooldown = 0.75f;
		float mLength = 146.25f;
		float mEdgeThickness = 6.f;
		float mOpeningAngleDegrees = 50.f;
		float mLateralKnockback = 75.f;
		float mElapsed = 0.f;
		movement::MovementInfluenceSourceId mMovementSourceId = 0;
	};

	bool RegisterLanceDriveActorType();
}
