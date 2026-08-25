#pragma once

#include "gameplay/ability/GameAbility.h"

namespace ly
{
	class FrostMaelstromFieldActor;

	class FrostMaelstromAbility final : public GameAbilityBehavior
	{
	public:
		bool Validate(
			const GameAbilityDefinition& definition,
			std::string* failureReason
		) const override;

		bool Activate(GameAbilityBehaviorContext& context) override;
		void Tick(GameAbilityBehaviorContext& context, float deltaTime) override;
		void End(
			GameAbilityBehaviorContext& context,
			sas::AbilityEndReason reason
		) override;

	private:
		void EmitEvent(
			GameAbilityBehaviorContext& context,
			const GameplayTag& eventTag
		) const;

		weak_ptr<FrostMaelstromFieldActor> mField;
	};
}
