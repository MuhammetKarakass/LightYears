#pragma once

#include "content/ContentId.h"
#include "framework/Actor.h"
#include "gameplay/damage/DamageContext.h"

#include <SFML/System/Vector2.hpp>

namespace ly
{
	class ScorchDriveFireSegmentActor;

	class ScorchDriveTrailCoordinatorActor final : public Actor
	{
	public:
		ScorchDriveTrailCoordinatorActor(
			World* world,
			Actor* owner,
			float fireDamage,
			float fireTickInterval,
			int burnThresholdTicks,
			float burnDuration,
			float burnTickInterval,
			float burnDamageRatio,
			const sas::ContentId& sourceAbilityId,
			const List<GameplayTag>& sourceAbilityTags,
			const List<GameplayTag>& damageTags
		);

		void Tick(float deltaTime) override;
		void SetCastActive(bool active) { mCastActive = active; }
		void AddSegment(const weak_ptr<ScorchDriveFireSegmentActor>& segment);

	private:
		void PruneSegments();
		void DestroySegments();
		void PerformCombatTick();
		bool IsOpposingCombatant(const Actor& source, const Actor& actor) const;

		// The owner can be destroyed while the trail segments are still alive.
		// A weak reference lets the coordinator terminate safely instead of
		// dereferencing the destroyed ship on a later world tick.
		weak_ptr<Actor> mOwner;
		List<weak_ptr<ScorchDriveFireSegmentActor>> mSegments;
		float mFireDamage = 0.f;
		float mFireTickInterval = 0.f;
		int mBurnThresholdTicks = 1;
		float mBurnDuration = 0.f;
		float mBurnTickInterval = 0.f;
		float mBurnDamageRatio = 0.f;
		float mTickAccumulator = 0.f;
		bool mCastActive = true;
		sas::ContentId mSourceAbilityId;
		List<GameplayTag> mSourceAbilityTags;
		List<GameplayTag> mDamageTags;
	};
}
