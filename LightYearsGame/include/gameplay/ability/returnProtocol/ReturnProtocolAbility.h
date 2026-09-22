#pragma once

#include "gameplay/ability/GameAbility.h"
#include "gameplay/projectile/ProjectileReflectionService.h"

namespace ly
{
	class ReturnProtocolVisualActor;

	class ReturnProtocolAbility final
		: public GameAbilityBehavior,
		  public ProjectileReflectionReceiver
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
		bool TryReflectIncomingProjectile(
			AbilityWorldActor& projectile,
			Actor& defender
		) override;

	private:
		float ResolveReflectDamageMultiplier(
			GameAbilityBehaviorContext& context
		) const;
		void EmitEvent(
			GameAbilityBehaviorContext& context,
			const GameplayTag& eventTag
		) const;
		void EmitReflectionEvent(Actor& defender, AbilityWorldActor& projectile) const;

		Actor* mOwner = nullptr;
		weak_ptr<ReturnProtocolVisualActor> mVisualActor;
		float mReflectDamageMultiplier = 1.f;
		bool mActive = false;
		// Owns the reflection registration. Declared last so it is destroyed before the
		// receiver half of this object, guaranteeing the registry never holds a pointer
		// to a partially destroyed behavior.
		ProjectileReflectionService::Registration mRegistration;
	};
}
