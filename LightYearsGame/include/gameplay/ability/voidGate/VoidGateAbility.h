#pragma once

#include "gameplay/ability/GameAbility.h"
#include "gameplay/portal/PortalTransferService.h"

namespace ly
{
	class VoidGatePortalActor;

	class VoidGateAbility final : public GameAbilityBehavior
	{
	public:
		bool Validate(
			const GameAbilityDefinition& definition,
			std::string* failureReason
		) const override;

		float ResolveActiveDuration(
			const GameAbilityBehaviorContext& context,
			float defaultDuration
		) const override;

		bool Activate(GameAbilityBehaviorContext& context) override;
		bool OnInputPressed(GameAbilityBehaviorContext& context) override;
		void Tick(GameAbilityBehaviorContext& context, float deltaTime) override;
		void End(
			GameAbilityBehaviorContext& context,
			sas::AbilityEndReason reason
		) override;

	private:
		weak_ptr<VoidGatePortalActor> SpawnPortal(
			GameAbilityBehaviorContext& context,
			const sf::Vector2f& location
		) const;
		void EmitEvent(
			GameAbilityBehaviorContext& context,
			const GameplayTag& eventTag
		) const;

		weak_ptr<VoidGatePortalActor> mPortalA;
		weak_ptr<VoidGatePortalActor> mPortalB;
		PortalTransferService::PairId mPairId = 0;
		bool mWaitingForPortalB = false;
		float mPortalBPlacementElapsed = 0.f;
		float mPortalBPlacementTimeout = 4.f;
	};
}
