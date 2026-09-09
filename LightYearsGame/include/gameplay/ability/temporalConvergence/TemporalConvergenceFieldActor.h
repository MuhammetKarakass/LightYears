#pragma once

#include "effects/GameplayEffectHandle.h"
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "presentation/ability/temporalConvergence/TemporalConvergencePresentationProfile.h"

#include <SFML/Graphics/CircleShape.hpp>

namespace ly
{
	class Combatant;
	class AreaTelegraphActor;

	class TemporalConvergenceFieldActor final : public AbilityWorldActor
	{
	public:
		TemporalConvergenceFieldActor(
			World* world,
			Actor* owner,
			const TemporalConvergencePresentationProfile& presentationProfile
		);

		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;
		void Destroy() override;
		void ConfigureFromAttributes(
			const sas::GameplayAttributeList& attributes
		) override;
		void ConfigureFromAbilityValues(
			const sas::GameplayAttributeList& values
		);
		void SetSnapshotTarget(const sf::Vector2f& targetLocation);

	private:
		enum class Phase { Dormant, Travelling, Field };

		struct SlowedTarget
		{
			weak_ptr<Actor> target;
			sas::GameplayEffectHandle handle;
		};

		void BeginTravel();
		void BeginField();
		void SpawnTargetTelegraph();
		void CompleteTargetTelegraph();
		void UpdateSlowedTargets();
		void ClearSlowedTargets();
		void TryTriggerPlayerEntry();
		void Trigger();
		void ApplyStun(Combatant& target);
		bool IsInsideField(const Actor& actor) const;
		bool IsSlowTracked(const Actor* target) const;

		TemporalConvergencePresentationProfile mPresentationProfile;
		Phase mPhase = Phase::Dormant;
		List<SlowedTarget> mSlowedTargets;
		weak_ptr<AreaTelegraphActor> mTargetTelegraph;
		sf::Vector2f mSnapshotTarget{};
		sf::CircleShape mDormantCore;
		sf::CircleShape mDormantGlow;
		float mPhaseAge = 0.f;
		float mVisualAge = 0.f;
		float mActorLifetime = 6.f;
		float mInitialDelay = 2.f;
		float mTravelSpeed = 1000.f;
		float mRadius = 300.f;
		float mFieldDuration = 2.f;
		float mSlowFraction = 0.40f;
		float mStunDuration = 2.f;
		float mGrantedShield = 120.f;
		float mOvershieldHoldDuration = 4.f;
		float mOvershieldDecayPerSecond = 100.f;
		bool mTargetConfigured = false;
		bool mTriggered = false;
	};

	bool RegisterTemporalConvergenceFieldActorType();
}
