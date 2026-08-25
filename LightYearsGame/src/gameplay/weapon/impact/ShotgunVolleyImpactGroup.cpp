#include "gameplay/weapon/impact/ShotgunVolleyImpactGroup.h"

#include "framework/Actor.h"
#include "gameplay/combat/Combatant.h"

#include <algorithm>

namespace ly
{
	ShotgunVolleyImpactGroup::ShotgunVolleyImpactGroup(
		Actor& source,
		const List<GameplayTag>& damageTags,
		float baseDamage,
		float damageReductionPerAdditionalHit,
		float minimumDamageMultiplier,
		const DamagePayload& payload
	)
		: mDamageTags{ damageTags }
		, mDamagePayload{ payload }
		, mBaseDamage{ std::max(0.f, baseDamage) }
		, mDamageReductionPerAdditionalHit{ std::clamp(damageReductionPerAdditionalHit, 0.f, 1.f) }
		, mMinimumDamageMultiplier{ std::clamp(minimumDamageMultiplier, 0.f, 1.f) }
	{
		if (const shared_ptr<Object> sourceObject = source.GetWeakPtr().lock())
		{
			mSource = std::dynamic_pointer_cast<Actor>(sourceObject);
		}
	}

	void ShotgunVolleyImpactGroup::AddPellet()
	{
		++mRemainingPellets;
	}

	void ShotgunVolleyImpactGroup::RegisterImpact(Actor& target)
	{
		if (target.GetIsPendingDestroy())
		{
			return;
		}

		int& hitCount = mTargetHitCounts[target.GetUniqueID()];
		const float multiplier = std::max(
			mMinimumDamageMultiplier,
			1.f - mDamageReductionPerAdditionalHit * static_cast<float>(hitCount)
		);
		++hitCount;
		const shared_ptr<Actor> source = mSource.lock();
		ApplyCombatDamage(
			target,
			mBaseDamage * multiplier,
			source.get(),
			mDamageTags,
			mDamagePayload
		);
	}

	void ShotgunVolleyImpactGroup::CompletePellet()
	{
		if (mRemainingPellets <= 0)
		{
			return;
		}

		--mRemainingPellets;
	}
}
