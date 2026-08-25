#pragma once

#include "gameplay/ability/GameAbility.h"

#include <SFML/System/Vector2.hpp>

namespace ly
{
	class FrozenThrongAbility final : public GameAbilityBehavior
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
		void OnGameplayEvent(
			GameAbilityBehaviorContext& context,
			const sas::AbilityEvent& event
		) override;

	private:
		sas::GameplayAttributeList ResolveValues(
			GameAbilityBehaviorContext& context
		) const;
		void SpawnHusksForKill(
			GameAbilityBehaviorContext& context,
			const sf::Vector2f& origin
		);
		void SpawnHusk(
			GameAbilityBehaviorContext& context,
			const sf::Vector2f& origin,
			const sas::GameplayAttributeList& values
		);
		void EmitEvent(
			GameAbilityBehaviorContext& context,
			const GameplayTag& eventTag
		) const;

		bool mActive = false;
	};
}
