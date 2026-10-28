#pragma once

#include "gameplay/ability/actors/AbilityWorldActor.h"

namespace vega
{
	struct HullShockDischarge
	{
		float damage = 0.f;
		float radius = 0.f;
		// Feature-local captured delivery shape; selected only by the active evolve.
		float sectorHalfAngle = 0.f;
		sf::Vector2f direction{ 1.f, 0.f };
		List<GameplayTag> damageTags;
		DamagePayload payload;
		sas::ContentId abilityId;
		List<GameplayTag> abilityTags;
	};
	int ApplyHullShockDischarge(Actor& owner, const HullShockDischarge& discharge);

	class HullShockAftershockActor final : public AbilityWorldActor
	{
	public:
		HullShockAftershockActor(World* world, Actor* owner, const HullShockDischarge& discharge, std::shared_ptr<const AbilityConfigurationContent> configuration);
		void Tick(float deltaTime) override;
	private:
		HullShockDischarge mDischarge;
		float mElapsed = 0.f;
		bool mFired = false;
	};
}
