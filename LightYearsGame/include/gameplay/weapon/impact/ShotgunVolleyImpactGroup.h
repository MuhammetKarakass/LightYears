#pragma once

#include "framework/Core.h"
#include "gameplay/damage/DamageContext.h"
#include "gameplay/weapon/impact/ProjectileImpactBehavior.h"

namespace ly
{
	class Actor;

	// Applies each pellet immediately. Per-target hit order supplies progressive
	// falloff without making the whole volley wait for its slowest projectile.
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
		weak_ptr<Actor> mSource;
		List<GameplayTag> mDamageTags;
		DamagePayload mDamagePayload;
		Map<unsigned int, int> mTargetHitCounts;
		float mBaseDamage = 0.f;
		float mDamageReductionPerAdditionalHit = 0.f;
		float mMinimumDamageMultiplier = 1.f;
		int mRemainingPellets = 0;
	};
}
