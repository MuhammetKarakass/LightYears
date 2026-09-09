#pragma once

#include "gameplay/ability/GameAbility.h"

namespace ly
{
	class ClosedCircuitAbility final : public GameAbilityBehavior
	{
	public:
		bool Validate(const GameAbilityDefinition& definition, std::string* failureReason) const override;
		bool Activate(GameAbilityBehaviorContext& context) override;
		void End(GameAbilityBehaviorContext& context, sas::AbilityEndReason reason) override;
	private:
		void EmitEvent(GameAbilityBehaviorContext& context, const GameplayTag& eventTag) const;
		struct PendingDelivery
		{
			sf::Vector2f target{};
			float speed = 280.f;
			float formationDuration = 0.50f;
			float radius = 250.f;
			float health = 0.f;
		};
		PendingDelivery mPendingDelivery;
		bool mHasPendingDelivery = false;
	};
}
