#pragma once

#include "attributes/AttributeSystem.h"
#include "content/ContentId.h"

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

		// Only physical, travelling projectile families override this marker.
		// Fields, beams and visual-only actors remain false by default so a
		// projectile-clearing ability cannot accidentally destroy them.
		virtual bool IsProjectileActor() const { return false; }

		Actor* GetOwnerActor() const { return mOwner; }

		void SetDamage(float damage) { mDamage = damage; }
		float GetDamage() const { return mDamage; }
		void SetDamageTags(const List<GameplayTag>& damageTags);
		const List<GameplayTag>& GetDamageTags() const { return mDamageTags; }
		void SetDamageAttributes(const sas::GameplayAttributeList& attributes);
		const DamagePayload& GetDamagePayload() const { return mDamagePayload; }
		void SetAbilityUpgradeIds(const List<std::string>& upgradeIds) { mAbilityUpgradeIds = upgradeIds; }
		const List<std::string>& GetAbilityUpgradeIds() const { return mAbilityUpgradeIds; }
		bool HasAbilityUpgrade(const std::string& upgradeId) const;
		void SetSourceAbility(
			const sas::ContentId& abilityId,
			const List<GameplayTag>& abilityTags
		)
		{
			mSourceAbilityId = abilityId;
			mSourceAbilityTags = abilityTags;
		}
		const sas::ContentId& GetSourceAbilityId() const { return mSourceAbilityId; }
		const List<GameplayTag>& GetSourceAbilityTags() const { return mSourceAbilityTags; }

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
		List<std::string> mAbilityUpgradeIds;
		sas::ContentId mSourceAbilityId;
		List<GameplayTag> mSourceAbilityTags;
		float mLifeTime;
		float mAge;
		float mCollisionRadius;
		bool mEnablePhysicsOnBeginPlay;
	};
}
