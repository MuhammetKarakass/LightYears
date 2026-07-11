#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/ability/AbilityEvent.h"
#include <algorithm>

namespace ly
{
	CombatRuntime::CombatRuntime(Actor& owner)
		: mOwner{ &owner },
		mAttributeSystem{},
		mOwnedTags{},
		mEffectSystem{ owner, mAttributeSystem, mOwnedTags },
		mAbilitySystem{ owner, mAttributeSystem, mEffectSystem, mOwnedTags }
	{
	}

	void CombatRuntime::InitializeFromShipDefinition(const ShipDefinition& shipDefinition)
	{
		mAttributeSystem.RegisterAttribute(OwnerAttributeIds::MaxHealth, shipDefinition.health);
		mAttributeSystem.RegisterAttribute(OwnerAttributeIds::AttackPower, 0.f);
		mAttributeSystem.RegisterAttribute(OwnerAttributeIds::AbilityHaste, 0.f);
		mAttributeSystem.RegisterAttribute(OwnerAttributeIds::MoveSpeedHorizontal, 0.f);
		mAttributeSystem.RegisterAttribute(OwnerAttributeIds::MoveSpeedVertical, 0.f);
		mAttributeSystem.RegisterAttribute(OwnerAttributeIds::Armor, 0.f);
		mAttributeSystem.RegisterAttribute(OwnerAttributeIds::Luck, 0.f);
	}

	void CombatRuntime::Tick(float deltaTime)
	{
		mEffectSystem.Tick(deltaTime);
		mAbilitySystem.Tick(deltaTime);
	}

	void CombatRuntime::Clear()
	{
		mAbilitySystem.Clear();
		mEffectSystem.Clear();
		mOwnedTags.Clear();
		mAttributeSystem.Clear();
	}

	void CombatRuntime::ProcessIncomingDamage(DamageContext& context)
	{
		mEffectSystem.ProcessIncomingDamage(context);
		for (const AbilityEvent& event : mEffectSystem.DrainPendingEvents())
		{
			mAbilitySystem.HandleGameplayEvent(event);
		}
		const float armor = std::clamp(mAttributeSystem.GetCurrentValue(OwnerAttributeIds::Armor), 0.f, 0.95f);
		const float damageBeforeArmor = context.remainingDamage;
		context.remainingDamage = std::max(0.f, damageBeforeArmor * (1.f - armor));
		context.mitigatedDamage += damageBeforeArmor - context.remainingDamage;
		context.modifiedDamage = context.remainingDamage;
		onDamageProcessed.Broadcast(context);
	}

	void CombatRuntime::NotifyDamageResolved(const DamageContext& context)
	{
		if (!mOwner || context.appliedDamage <= 0.f)
		{
			return;
		}

		AbilityEvent event;
		event.eventTag = GameplayTag{ "Event.Owner.DamageTaken" };
		event.source = context.source;
		event.target = context.target;
		event.magnitude = context.appliedDamage;
		event.damageContext = &context;
		mAbilitySystem.HandleGameplayEvent(event);
		onDamageResolved.Broadcast(context);
	}
}


