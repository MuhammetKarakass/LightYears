#pragma once

#include "gameplay/ability/GameAbility.h"

namespace ly
{
	class AstralSurgeAbility final : public GameAbilityBehavior
	{
	public:
		bool Validate(
			const GameAbilityDefinition& definition,
			std::string* failureReason = nullptr
		) const override;

		bool Activate(GameAbilityBehaviorContext& context) override;
		void Tick(GameAbilityBehaviorContext& context, float deltaTime) override;
		void End(
			GameAbilityBehaviorContext& context,
			sas::AbilityEndReason reason
		) override;

	private:
		void Fire(GameAbilityBehaviorContext& context);
		void RemoveFocusLocks(GameAbilityBehaviorContext& context) const;
		void EmitEvent(
			GameAbilityBehaviorContext& context,
			const GameplayTag& eventTag
		) const;

		sf::Vector2f mDirection{ 0.f, -1.f };
		float mFocusElapsed = 0.f;
		bool mStarted = false;
		bool mFired = false;
	};
}
