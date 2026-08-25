#pragma once

#include "gameplay/ability/GameAbilityActionExecutor.h"
#include "gameplay/combat/summon/SummonedCombatantActor.h"
#include "presentation/ability/combatSentry/CombatSentryPresentationProfile.h"

namespace ly
{
	class CombatSentryActor final : public SummonedCombatantActor
	{
	public:
		struct Configuration
		{
			LightYearsAbilitySystemComponent* abilitySystem = nullptr;
			// A sentry can outlive an instant invocation (notably Echo). Keep the
			// resolved ability definition as a value-owned snapshot instead of
			// retaining a pointer to an invocation that may already be destroyed.
			shared_ptr<const GameAbilityDefinition> abilityDefinition;
		};

		CombatSentryActor(
			World* world,
			Actor* owner,
			const CombatSentryTurretPresentationProfile& profile,
			Configuration configuration
		);

		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;
		void ConfigureFromAttributes(
			const sas::GameplayAttributeList& attributes
		) override;

	private:
		shared_ptr<Actor> FindTarget() const;
		void TryFire(float deltaTime);
		float ResolveProjectileDamage() const;
		float ResolveAttackRate() const;

		CombatSentryTurretPresentationProfile mProfile;
		Configuration mConfiguration;
		float mTargetingRange = 800.f;
		float mBaseDamage = 14.f;
		float mOwnerAttackPowerScale = 0.60f;
		float mBaseAttackRate = 1.25f;
		float mFireCooldown = 0.f;
		float mVisualAge = 0.f;
	};

	bool RegisterCombatSentryTurretActorType();
}
