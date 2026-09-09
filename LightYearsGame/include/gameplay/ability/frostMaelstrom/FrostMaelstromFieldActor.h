#pragma once

#include "attributes/AttributeSystem.h"
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameplay/movement/MovementInfluenceTypes.h"
#include "presentation/ability/frostMaelstrom/FrostMaelstromPresentationProfile.h"

#include <SFML/Graphics/CircleShape.hpp>

#include <cstdint>
#include <vector>

namespace ly
{
	class SpaceShip;

	class FrostMaelstromFieldActor final : public AbilityWorldActor
	{
	public:
		FrostMaelstromFieldActor(
			World* world,
			Actor* owner,
			const FrostMaelstromPresentationProfile& presentationProfile
		);

		void BeginPlay() override;
		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;
		void Destroy() override;
		void ConfigureFromAttributes(
			const sas::GameplayAttributeList& attributes
		) override;

		// Ability values are resolved from the ability instance after the generic
		// actor spawner has applied common actor metadata.
		void ConfigureFromAbilityValues(
			const sas::GameplayAttributeList& values
		);
		// Ends this field's influence without cancelling the target's resulting
		// velocity; MovementComponent then applies ordinary drift damping.
		void StopControl();

		float GetCurrentRadius() const { return mCurrentRadius; }
		float GetCurrentSpeed() const { return mCurrentSpeed; }

	private:
		struct ControlledTarget
		{
			weak_ptr<SpaceShip> target;
		};

		void AdvanceField(float deltaTime);
		void UpdateControlledTargets();
		void ApplyControlForces();
		void ApplyCryoTick();
		void AddTarget(const shared_ptr<SpaceShip>& target);
		bool ContainsTarget(const SpaceShip* target) const;
		void RemoveControl(ControlledTarget& controlled);
		bool IsEligibleTarget(const Actor& target) const;
		sf::Vector2f ResolveControlAcceleration(
			const ControlledTarget& controlled
		) const;
		movement::MovementInfluenceSourceId GetControlSourceId() const;
		void ConfigureGeometry();

		FrostMaelstromPresentationProfile mPresentationProfile;
		float mDuration = 8.f;
		float mMinimumRadius = 300.f;
		float mMaximumRadius = 800.f;
		float mCurrentRadius = 300.f;
		float mMinimumSpeed = 200.f;
		float mMaximumSpeed = 500.f;
		float mCurrentSpeed = 200.f;
		float mTickDamage = 2.f;
		float mTickInterval = 0.25f;
		float mCryoStacksPerTick = 1.f;
		float mOrbitalAngularSpeed = 4.5f;
		float mInwardForce = 1500.f;
		float mOrbitalRadiusRatio = 0.42f;
		float mEnergyMaxReference = 50.f;
		float mEnergyMaxDamageScale = 0.005f;
		float mEnergyMaxRadiusScale = 0.20f;
		float mFieldAge = 0.f;
		float mTickAccumulator = 0.f;
		float mVisualAge = 0.f;
		sf::Vector2f mFieldDirection{ 0.f, -1.f };
		std::vector<ControlledTarget> mControlledTargets;

		sf::CircleShape mOuterRing;
		sf::CircleShape mInnerRing;
		inline static constexpr float ControlVelocityResponsePerSecond = 2.f;
	};

	bool RegisterFrostMaelstromFieldActorType();
}
