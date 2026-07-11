#pragma once

#include "framework/Actor.h"
#include "gameConfigs/AbilityActorStructs.h"

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
		void SetDamageTags(const List<GameplayTag>& damageTags) { mDamageTags = damageTags; }
		const List<GameplayTag>& GetDamageTags() const { return mDamageTags; }

		void SetLifeTime(float lifeTime) { mLifeTime = lifeTime; }
		float GetLifeTime() const { return mLifeTime; }
		float GetAge() const { return mAge; }

		void SetAbilityCollisionRadius(float radius);
		void SetAbilityPhysicsEnabled(bool enabled) { mEnablePhysicsOnBeginPlay = enabled; }
		void ConfigureCollisionFromOwner();

		virtual void ConfigureFromAttributes(const GameplayAttributeList& attributes);

	protected:
		bool IsValidAbilityTarget(const Actor* actor) const;
		void ApplyCombatDamageInRadius(
			const sf::Vector2f& center,
			float radius,
			float damageMultiplier = 1.f
		);

	private:
		Actor* mOwner;
		float mDamage;
		List<GameplayTag> mDamageTags;
		float mLifeTime;
		float mAge;
		float mCollisionRadius;
		bool mEnablePhysicsOnBeginPlay;
	};
}


