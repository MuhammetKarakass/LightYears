#pragma once

#include "gameplay/ability/GameAbility.h"

#include <cstddef>

namespace ly
{
	class OrbitingDroneActor;

	// The behavior owns the formation lifecycle. Each OrbitingDroneActor owns its
	// own orbit motion and per-target contact cooldown, keeping this class focused
	// on ability validation, resolved attributes, spawning, and cleanup.
	class OrbitalDronesAbility final : public GameAbilityBehavior
	{
	public:
		bool Validate(
			const GameAbilityDefinition& definition,
			std::string* failureReason = nullptr
		) const override;

		bool Activate(GameAbilityBehaviorContext& context) override;
		void End(
			GameAbilityBehaviorContext& context,
			sas::AbilityEndReason reason
		) override;

		float ResolveActiveDuration(
			const GameAbilityBehaviorContext& context,
			float defaultDuration
		) const override;

	private:
		void EmitEvent(
			GameAbilityBehaviorContext& context,
			const GameplayTag& eventTag
		) const;
		void DestroyDrones();

		List<weak_ptr<OrbitingDroneActor>> mDrones;
		bool mActive = false;
	};
}
