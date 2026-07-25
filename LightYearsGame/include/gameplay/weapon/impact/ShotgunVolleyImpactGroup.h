#pragma once

#include "framework/Core.h"
#include "gameplay/damage/DamageContext.h"
#include "gameplay/weapon/impact/ProjectileImpactBehavior.h"

namespace ly
{
	class Actor;

	// Resolves per-target pellet falloff after every projectile in one shotgun volley completes.
	class ShotgunVolleyImpactGroup final : public ProjectileImpactBehavior
	{
	public:
		ShotgunVolleyImpactGroup(
			Actor& source,
			const List<GameplayTag>& damageTags,
			float baseDamage,
			float damageReductionPerAdditionalHit,
			float minimumDamageMultiplier,
			const DamagePayload& payload = {}
		);

		void AddPellet();
		void RegisterImpact(Actor& target);
		void CompletePellet();
		void OnProjectileSpawned() override { AddPellet(); }
		bool HandleImpact(Actor& target) override
		{
			RegisterImpact(target);
			return true;
		}
		void OnProjectileFinished() override { CompletePellet(); }

	private:
		struct TargetImpactRecord
		{
			weak_ptr<Actor> target;
			int hitCount = 0;
		};

		void ResolveDamage();

		weak_ptr<Actor> mSource;
		List<GameplayTag> mDamageTags;
		DamagePayload mDamagePayload;
		Map<unsigned int, TargetImpactRecord> mTargetImpacts;
		float mBaseDamage = 0.f;
		float mDamageReductionPerAdditionalHit = 0.f;
		float mMinimumDamageMultiplier = 1.f;
		int mRemainingPellets = 0;
		bool mResolved = false;
	};
}
