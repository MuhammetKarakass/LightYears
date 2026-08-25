#pragma once

#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/HealthComponent.h"

namespace ly
{
	// Shared base for player-owned summons that are real combat targets. It keeps
	// life/armor/critical ownership outside individual abilities while leaving
	// each concrete summon free to implement its own movement and attacks.
	class SummonedCombatantActor : public AbilityWorldActor, public Combatant
	{
	public:
		struct CombatantConfiguration
		{
			float maxHealthSnapshot = 1.f;
			float baseArmor = 0.f;
			float ownerArmorScale = 0.f;
			float ownerCriticalChanceScale = 1.f;
		};

		SummonedCombatantActor(World* world, Actor* owner);
		~SummonedCombatantActor() override;

		void BeginPlay() override;
		void Tick(float deltaTime) override;
		void ApplyDamage(float amount) override;
		void ReceiveDamage(DamageContext context) override;

		CombatRuntime& GetCombatRuntime() override { return mCombatRuntime; }
		const CombatRuntime& GetCombatRuntime() const override { return mCombatRuntime; }
		HealthComponent& GetHealthComponent() { return mHealthComponent; }
		const HealthComponent& GetHealthComponent() const { return mHealthComponent; }

		void ConfigureCombatant(const CombatantConfiguration& configuration);
		float GetOwnerCombatAttribute(const sas::AttributeId& attributeId) const;
		float GetOwnerAttackSpeedMultiplier() const;

	protected:
		void ConfigureFriendlySummonCollision();

	private:
		void SynchronizeDynamicOwnerCombatAttributes();
		void OnHealthEmpty();

		HealthComponent mHealthComponent;
		CombatRuntime mCombatRuntime;
		CombatantConfiguration mConfiguration;
	};
}
