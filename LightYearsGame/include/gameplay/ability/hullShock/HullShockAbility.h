#pragma once

#include "gameplay/ability/GameAbility.h"
#include "effects/GameplayEffectRuntimeEntry.h"
#include "gameplay/combat/IncomingDamageWindow.h"

namespace vega
{
	class AreaTelegraphActor;
	class HullShockConeVisualActor;

	class HullShockAbility final : public GameAbilityBehavior
	{
	public:
		bool Validate(const GameAbilityDefinition& definition, std::string* failureReason) const override;

		bool Activate(GameAbilityBehaviorContext& context) override;
		void Tick(GameAbilityBehaviorContext& context, float deltaTime) override;
		void End(GameAbilityBehaviorContext& context, sas::AbilityEndReason reason) override;

	private:
		void Discharge(GameAbilityBehaviorContext& context);
		void EndCapacitor(GameAbilityBehaviorContext& context);
		void EmitEvent(GameAbilityBehaviorContext& context, const GameplayTag& eventTag) const;

		weak_ptr<AreaTelegraphActor> mTelegraph;
		weak_ptr<HullShockConeVisualActor> mConeTelegraph;
		unique_ptr<IncomingDamageWindow> mIncomingDamage;
		sas::GameplayEffectHandle mReduction;
		sf::Vector2f mAimDirection{ 1.f, 0.f };
		float mFocusElapsed = 0.f;
		bool mDischarged = false;
	};
}
