#pragma once

#include "AbilitySystemInterface.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/control/ControlResponse.h"
#include "gameplay/damage/DamageContext.h"

namespace ly
{
	class Actor;
	class CombatRuntime;

	class Combatant : public sas::AbilitySystemInterface
	{
	public:
		virtual ~Combatant() = default;
		virtual CombatRuntime& GetCombatRuntime() = 0;
		virtual const CombatRuntime& GetCombatRuntime() const = 0;
		virtual ControlResponse ResolveControlResponse(
			const GameplayTag& controlTag
		) const
		{
			(void)controlTag;
			return {};
		}
		LightYearsAbilitySystemComponent&
			GetAbilitySystemComponent() override;
		virtual const LightYearsAbilitySystemComponent&
			GetAbilitySystemComponent() const override;
		virtual void ReceiveDamage(DamageContext context) = 0;
	};

	void ApplyCombatDamage(
		Actor& target,
		float damage,
		Actor* source = nullptr,
		const List<GameplayTag>& damageTags = {}
	);

	void ApplyCombatDamage(
		Actor& target,
		float damage,
		Actor* source,
		const List<GameplayTag>& damageTags,
		const DamagePayload& payload
	);

	void ApplyCombatDamage(
		Actor& target,
		float damage,
		Actor* source,
		const List<GameplayTag>& damageTags,
		const DamagePayload& payload,
		const sas::ContentId& sourceAbilityId,
		const List<GameplayTag>& sourceAbilityTags,
		DamageDeliveryType deliveryType = DamageDeliveryType::Direct,
		Actor* deliveryActor = nullptr
	);

	// Contact damage is emitted by several actor families. Keeping this gate
	// here prevents each ship/obstacle from knowing Energy Spear's lifecycle.
	bool CanApplyContactDamage(const Actor& source, const Actor& target);
}
