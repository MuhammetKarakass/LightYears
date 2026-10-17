#pragma once
#include "gameplay/weapon/projectile/PrimaryWeaponProjectileActor.h"
#include "gameplay/module/generated/ModuleGeneratedLease.h"
#include <optional>

namespace vega
{
	struct ModuleRicochetProjectileSnapshot
	{
		WeaponPresentationDefinition presentation;
		sas::GameplayAttributeList attributes;
		float speed = 0.f;
	};
	class ModuleRicochetProjectileActor final : public PrimaryWeaponProjectileActor
	{
	public:
		ModuleRicochetProjectileActor(World* world, Actor* owner, const ModuleRicochetProjectileSnapshot& snapshot, const std::shared_ptr<ModuleGeneratedLease>& lease, const List<std::uint64_t>& visited, float range, float damage, const List<GameplayTag>& tags, float secondaryDamage, const std::optional<sas::GameplayEffectSpec>& status);
		static shared_ptr<ModuleRicochetProjectileActor> Spawn(Actor& source, const ModuleRicochetProjectileSnapshot& snapshot, const sf::Vector2f& origin, const std::shared_ptr<ModuleGeneratedLease>& lease, const List<std::uint64_t>& visited, float range, float damage, const List<GameplayTag>& tags, float secondaryDamage, const std::optional<sas::GameplayEffectSpec>& status);
		void Tick(float deltaTime) override;
		void OnActorBeginOverlap(Actor* target) override;
		bool CanBeCapturedByRelay() const override { return false; }
		bool CanBeReflected() const override { return false; }
		bool HasSelectedTarget() const { return !mTarget.expired(); }
	private:
		ModuleRicochetProjectileSnapshot mSnapshot;
		weak_ptr<ModuleGeneratedLease> mLease;
		List<std::uint64_t> mVisited;
		weak_ptr<Actor> mTarget;
		float mRange;
		float mSecondaryDamage;
		std::optional<sas::GameplayEffectSpec> mStatus;
	};
}
