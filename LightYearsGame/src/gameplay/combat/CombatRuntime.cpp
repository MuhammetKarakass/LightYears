#include "gameplay/combat/CombatRuntime.h"
#include "framework/Actor.h"
#include "gameplay/ability/AbilityEvent.h"
#include "gameplay/attachment/AttachmentDefinition.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/attributes/AttributeMath.h"
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

	void CombatRuntime::InitializeOwnerAttributes(float maxHealth)
	{
		mAttributeSystem.RegisterAttribute(OwnerAttributeIds::MaxHealth, maxHealth);
		mAttributeSystem.RegisterAttribute(OwnerAttributeIds::HealthRegen, 0.f);
		mAttributeSystem.RegisterAttribute(OwnerAttributeIds::EnergyMax, 0.f);
		mAttributeSystem.RegisterAttribute(OwnerAttributeIds::EnergyRegen, 0.f);
		mAttributeSystem.RegisterAttribute(OwnerAttributeIds::AttackPower, 0.f);
		mAttributeSystem.RegisterAttribute(OwnerAttributeIds::AttackSpeed, 0.f);
		mAttributeSystem.RegisterAttribute(OwnerAttributeIds::AbilityHaste, 0.f);
		mAttributeSystem.RegisterAttribute(OwnerAttributeIds::MoveSpeedHorizontal, 0.f);
		mAttributeSystem.RegisterAttribute(OwnerAttributeIds::MoveSpeedVertical, 0.f);
		mAttributeSystem.RegisterAttribute(OwnerAttributeIds::Armor, 0.f);
		mAttributeSystem.RegisterAttribute(OwnerAttributeIds::Luck, 0.f);
		mAttributeSystem.RegisterAttribute(OwnerAttributeIds::CriticalChance, 0.f);
	}

	float CombatRuntime::GetCriticalChance() const
	{
		return AttributeMath::GetCriticalChance(mAttributeSystem.GetCurrentValue(OwnerAttributeIds::CriticalChance));
	}

	float CombatRuntime::GetCombatLuckFactor() const
	{
		return AttributeMath::GetCombatLuckFactor(mAttributeSystem.GetCurrentValue(OwnerAttributeIds::Luck));
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
		const float armorReduction = AttributeMath::GetArmorDamageReduction(
			mAttributeSystem.GetCurrentValue(OwnerAttributeIds::Armor)
		);
		const float effectiveArmor = armorReduction * (1.f - context.payload.armorPenetration);
		const float damageBeforeArmor = context.remainingDamage;
		context.remainingDamage = std::max(0.f, damageBeforeArmor * (1.f - effectiveArmor));
		context.mitigatedDamage += damageBeforeArmor - context.remainingDamage;
		context.modifiedDamage = context.remainingDamage;

		const List<GameplayTag> appliedStatuses = DamageTypeSystem::ApplyStatusEffects(mEffectSystem, context);
		if (auto* sourceCombatant = context.source ? dynamic_cast<Combatant*>(context.source) : nullptr)
		{
			for (const GameplayTag& status : appliedStatuses)
			{
				if (status != DamageStatusSchema::Ignite)
				{
					continue;
				}
				AbilityEvent event;
				event.eventTag = AttachmentSchema::Event::SourceIgniteApplied;
				event.source = context.source;
				event.target = context.target;
				event.magnitude = context.remainingDamage;
				event.damageContext = &context;
				sourceCombatant->GetCombatRuntime().GetAbilities().HandleGameplayEvent(event);
			}
		}
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

		if (auto* sourceCombatant = context.source ? dynamic_cast<Combatant*>(context.source) : nullptr)
		{
			AbilityEvent sourceEvent;
			sourceEvent.eventTag = AttachmentSchema::Event::SourceDamageDealt;
			sourceEvent.source = context.source;
			sourceEvent.target = context.target;
			sourceEvent.magnitude = context.appliedDamage;
			sourceEvent.damageContext = &context;
			sourceCombatant->GetCombatRuntime().GetAbilities().HandleGameplayEvent(sourceEvent);
		}
		onDamageResolved.Broadcast(context);
	}
}


