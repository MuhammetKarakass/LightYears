#pragma once

#include "attributes/AttributeSystem.h"
#include "abilities/AbilityHandle.h"
#include "content/ContentId.h"

#include "framework/Actor.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/damage/DamageContext.h"
#include "gameplay/projectile/ProjectileRelayParticipant.h"
#include "gameplay/projectile/ProjectileReflectionParticipant.h"
#include "gameplay/portal/PortalTransferParticipant.h"

#include <algorithm>

namespace ly
{
	class GameAbility;
	class AbilityWorldActor
		: public Actor,
		  public ProjectileRelayParticipant,
		  public ProjectileReflectionParticipant,
		  public PortalTransferParticipant
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

		Actor* GetOwnerActor() const;
		shared_ptr<Actor> LockOwnerActor() const { return mOwner.lock(); }
		// Reflection preserves the initial hostile source separately from the
		// current owner, which becomes the defender after a successful return.
		Actor* GetOriginalProjectileOwner() const;

		void SetDamage(float damage) { mDamage = damage; }
		float GetDamage() const { return mDamage; }
		void SetDamageTags(const List<GameplayTag>& damageTags);
		const List<GameplayTag>& GetDamageTags() const { return mDamageTags; }
		void SetDamageAttributes(const sas::GameplayAttributeList& attributes);
		const sas::GameplayAttributeList& GetDamageAttributes() const
		{
			return mDamageAttributes;
		}
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
		void SetSourceAbilityInstance(GameAbility* instance);
		GameAbility* GetSourceAbilityInstance() const;

		void SetProjectileRelayLineage(const ProjectileRelayLineage& lineage)
		{
			mProjectileRelayLineage = lineage;
		}
		const ProjectileRelayLineage& GetProjectileRelayLineage() const
		{
			return mProjectileRelayLineage;
		}

		void SetLifeTime(float lifeTime) { mLifeTime = lifeTime; }
		float GetLifeTime() const { return mLifeTime; }
		float GetAge() const { return mAge; }

		void SetAbilityCollisionRadius(float radius);
		float GetPhysicsCollisionRadius() const override { return mCollisionRadius; }
		// Relay clones intentionally use a separate collision policy. This keeps
		// self/friendly damage isolated from normal player and enemy projectiles.
		void SetRelayProjectileDamagePolicy(bool allowFriendlyFire);
		void ConfigureRelayClone(const ProjectileRelayCloneRequest& request);
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

		bool CanBeCapturedByRelay() const override { return IsProjectileActor(); }
		// Physical projectile families explicitly opt in by overriding these two
		// methods. A field or delivery actor must never become reflectable merely
		// because it derives from AbilityWorldActor.
		bool CanBeReflected() const override { return false; }
		bool TryReflectProjectile(
			const ProjectileReflectionRequest& request
		) override;
		Actor& GetPortalTransferActor() override { return *this; }
		bool CanEnterPortalTransfer() const override
		{
			return IsProjectileActor() && !GetIsPendingDestroy();
		}
		float GetPortalTransferRadius() const override
		{
			return std::max(0.1f, mCollisionRadius);
		}
		bool IsInPortalTransit() const override { return mPortalTransit; }
		void BeginPortalTransit() override;
		void CompletePortalTransit(const sf::Vector2f& exitLocation) override;
		bool BuildRelaySnapshot(ProjectileRelaySnapshot& snapshot) const override;
		weak_ptr<AbilityWorldActor> SpawnRelayClone(
			const ProjectileRelayCloneRequest& request
		) const override;

	protected:
		// Must run before a projectile resolves an ordinary target hit. It is kept
		// in the common actor layer so concrete projectile families share one
		// receiver lookup rather than knowing Return Protocol directly.
		bool TryReflectOnOverlap(Actor* otherActor);
		// Direct-flight projectile families keep their own trajectory state, but
		// ownership transfer, damage scaling and visual direction are universal.
		// This helper leaves only the family-owned trajectory field to update.
		bool ApplyBallisticReflection(
			const ProjectileReflectionRequest& request,
			float speed,
			sf::Vector2f& inOutTrajectory
		);
		void ApplyReflectionOwnership(
			Actor& newOwner,
			float damageMultiplier
		);
		bool IsValidAbilityTarget(const Actor* actor) const;
		void ApplyCombatDamageInRadius(
			const sf::Vector2f& center,
			float radius,
			float damageMultiplier = 1.f
		);

	private:
		void RebuildDamagePayload();

		weak_ptr<Actor> mOwner;
		// Some isolated engine tests construct an owner on the stack. Runtime
		// actors use mOwner; this fallback is populated only when Object has no
		// shared ownership and therefore cannot produce a weak reference.
		Actor* mUnmanagedOwner = nullptr;
		weak_ptr<Actor> mOriginalProjectileOwner;
		Actor* mUnmanagedOriginalProjectileOwner = nullptr;
		bool mHasOriginalProjectileOwner = false;
		float mDamage;
		List<GameplayTag> mDamageTags;
		sas::GameplayAttributeList mDamageAttributes;
		DamagePayload mDamagePayload;
		List<std::string> mAbilityUpgradeIds;
		sas::ContentId mSourceAbilityId;
		List<GameplayTag> mSourceAbilityTags;
		sas::AbilityHandle mSourceAbilityHandle;
		ProjectileRelayLineage mProjectileRelayLineage;
		float mLifeTime;
		float mAge;
		sf::Vector2f mPreviousLocation;
		float mCollisionRadius;
		bool mAllowFriendlyFire;
		bool mEnablePhysicsOnBeginPlay;
		bool mPortalTransit = false;
		bool mPortalPhysicsWasEnabled = false;
		CollisionLayer mPortalCollisionLayer = CollisionLayer::None;
		CollisionLayer mPortalCollisionMask = CollisionLayer::None;
		sf::Vector2f mPortalVelocity{};
	};
}
