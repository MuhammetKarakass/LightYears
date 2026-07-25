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
		if (!mResolved)
		{
			++mRemainingPellets;
		}
	}

	void ShotgunVolleyImpactGroup::RegisterImpact(Actor& target)
	{
		if (mResolved || target.GetIsPendingDestroy())
		{
			return;
		}

		const shared_ptr<Object> targetObject = target.GetWeakPtr().lock();
		const shared_ptr<Actor> targetActor = std::dynamic_pointer_cast<Actor>(targetObject);
		if (!targetActor)
		{
			return;
		}

		TargetImpactRecord& record = mTargetImpacts[target.GetUniqueID()];
		record.target = targetActor;
		++record.hitCount;
	}

	void ShotgunVolleyImpactGroup::CompletePellet()
	{
		if (mResolved || mRemainingPellets <= 0)
		{
			return;
		}

		--mRemainingPellets;
		if (mRemainingPellets == 0)
		{
			mResolved = true;
			ResolveDamage();
		}
	}

	void ShotgunVolleyImpactGroup::ResolveDamage()
	{
		const shared_ptr<Actor> source = mSource.lock();
		for (const auto& [targetId, record] : mTargetImpacts)
		{
			(void)targetId;
			const shared_ptr<Actor> target = record.target.lock();
			if (!target || target->GetIsPendingDestroy() || record.hitCount <= 0)
			{
				continue;
			}

			const float multiplier = std::max(
				mMinimumDamageMultiplier,
				1.f - mDamageReductionPerAdditionalHit * static_cast<float>(record.hitCount - 1)
			);
			const float damagePerPellet = mBaseDamage * multiplier;
			for (int hitIndex = 0; hitIndex < record.hitCount && !target->GetIsPendingDestroy(); ++hitIndex)
			{
				ApplyCombatDamage(*target, damagePerPellet, source.get(), mDamageTags, mDamagePayload);
			}
		}
	}
}
