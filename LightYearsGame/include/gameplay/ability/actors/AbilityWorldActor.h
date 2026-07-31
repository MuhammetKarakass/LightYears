#pragma once

#include "attributes/AttributeSystem.h"

#include "framework/Actor.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/damage/DamageContext.h"

namespace ly
{
	class AbilityWorldActor : public Actor
	{
	public:
		AbilityWorldActor(World* world, Actor* owner, const std::string& texturePath = "");

		virtual void BeginPlay() override;
		virtual void Tick(float deltaTime) override;
		virtual void OnActorBeginOverlap(Actor* otherActor) override;

		Actor* GetOwnerActor() const { return mOwner; }

		void SetDamage(float damage) { mDamage = damage; }
		float GetDamage() const { return mDamage; }
		void SetDamageTags(const List<GameplayTag>& damageTags);
		const List<GameplayTag>& GetDamageTags() const { return mDamageTags; }
		void SetDamageAttributes(const sas::GameplayAttributeList& attributes);
		const DamagePayload& GetDamagePayload() const { return mDamagePayload; }
		void SetAbilityUpgradeIds(const List<GameplayTag>& upgradeIds) { mAbilityUpgradeIds = upgradeIds; }
		const List<GameplayTag>& GetAbilityUpgradeIds() const { return mAbilityUpgradeIds; }
		bool HasAbilityUpgrade(const GameplayTag& upgradeId) const;

		void SetLifeTime(float lifeTime) { mLifeTime = lifeTime; }
		float GetLifeTime() const { return mLifeTime; }
		float GetAge() const { return mAge; }

		void SetAbilityCollisionRadius(float radius);
		void SetAbilityPhysicsEnabled(bool enabled)
		{
			mEnablePhysicsOnBeginPlay = enabled;
			if (!enabled)
			{
				SetEnablePhysics(false);
			}
		}
		void ConfigureCollisionFromOwner();

		virtual void ConfigureFromAttributes(const sas::GameplayAttributeList& attributes);

	protected:
		bool IsValidAbilityTarget(const Actor* actor) const;
		void ApplyCombatDamageInRadius(
			const sf::Vector2f& center,
			float radius,
			float damageMultiplier = 1.f
		);

	private:
		void RebuildDamagePayload();

		Actor* mOwner;
		float mDamage;
		List<GameplayTag> mDamageTags;
		sas::GameplayAttributeList mDamageAttributes;
		DamagePayload mDamagePayload;
		List<GameplayTag> mAbilityUpgradeIds;
		float mLifeTime;
		float mAge;
		float mCollisionRadius;
		bool mEnablePhysicsOnBeginPlay;
	};
}


