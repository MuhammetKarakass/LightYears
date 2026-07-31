#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/CombatRuntime.h"
#include "framework/Actor.h"
#include "abilities/AbilityEvent.h"
#include "gameplay/attachment/AttachmentDefinition.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "attributes/AttributeMath.h"
#include <algorithm>
#include <vector>

namespace ly
{
	CombatRuntime::CombatRuntime(Actor& owner)
		: mOwner{ owner },
		mAbilitySystemComponent{ owner },
		mEffectPresentation{ owner }
	{
		sas::AbilitySystemComponent::EffectCallbacks callbacks;
		callbacks.addStack =
			[](sas::ActiveGameplayEffect& effect)
			{
				return LightYearsAbilitySystemComponent::
					GetEffectBehaviorRuntime().AddStack(effect);
			};
		callbacks.tick =
			[this](sas::ActiveGameplayEffect& effect, float deltaTime)
			{
				return LightYearsAbilitySystemComponent::
					GetEffectBehaviorRuntime().Tick(
						effect,
						mOwner,
						deltaTime
					);
			};
		callbacks.behaviorEvent =
			[this](const sas::GameplayEffectBehaviorEvent& event)
			{
				QueueEffectEvent(event, mProcessingDamageContext);
			};
		callbacks.activated =
			[this](sas::ActiveGameplayEffect& effect)
			{
				mEffectPresentation.Activate(effect);
			};
		callbacks.changed =
			[this](sas::ActiveGameplayEffect& effect)
			{
				mEffectPresentation.Synchronize(effect);
			};
		callbacks.removing =
			[this](sas::ActiveGameplayEffect& effect)
			{
				mEffectPresentation.Remove(effect);
			};
		mAbilitySystemComponent.SetEffectRuntimeCallbacks(std::move(callbacks));
	}

	void CombatRuntime::InitializeOwnerAttributes(float maxHealth)
	{
		mAbilitySystemComponent.InitializeOwnerAttributes(maxHealth);
	}

	float CombatRuntime::GetCriticalChance() const
	{
		return sas::AttributeMath::GetCriticalChance(
			mAbilitySystemComponent.GetAttributes().GetCurrentValue(
				OwnerAttributeIds::CriticalChance
			)
		);
	}

	float CombatRuntime::GetCombatLuckFactor() const
	{
		return sas::AttributeMath::GetCombatLuckFactor(
			mAbilitySystemComponent.GetAttributes().GetCurrentValue(
				OwnerAttributeIds::Luck
			)
		);
	}

	void CombatRuntime::Tick(float deltaTime)
	{
		mAbilitySystemComponent.Tick(deltaTime);
		DispatchPendingEffectEvents();
	}

	void CombatRuntime::Clear()
	{
		mAbilitySystemComponent.Clear();
		mEffectPresentation.Clear();
		mPendingEffectEvents.clear();
	}

	void CombatRuntime::ProcessIncomingDamage(DamageContext& context)
	{
		using IncomingDamagePhase =
			LightYearsAbilitySystemComponent::IncomingDamagePhase;

		mProcessingDamageContext = &context;
		mAbilitySystemComponent.ProcessGameplayEffectEvent(
			context,
			std::vector<IncomingDamagePhase>{
				IncomingDamagePhase::PreMitigation,
				IncomingDamagePhase::Standard
			},
			[](const sas::ActiveGameplayEffect& effect)
			{
				return LightYearsAbilitySystemComponent::
					GetEffectBehaviorRuntime().GetEventPhase(
					effect,
					IncomingDamagePhase::Standard
				);
			},
			[](sas::ActiveGameplayEffect& effect, DamageContext& damageContext)
			{
				return LightYearsAbilitySystemComponent::
					GetEffectBehaviorRuntime().ProcessEvent(
					effect,
					damageContext
				);
			},
			[](const DamageContext& damageContext)
			{
				return damageContext.remainingDamage > 0.f;
			}
		);
		mProcessingDamageContext = nullptr;
		DispatchPendingEffectEvents();
		const float armorReduction = sas::AttributeMath::GetArmorDamageReduction(
			mAbilitySystemComponent.GetAttributes().GetCurrentValue(
				OwnerAttributeIds::Armor
			)
		);
		const float effectiveArmor = armorReduction * (1.f - context.payload.armorPenetration);
		const float damageBeforeArmor = context.remainingDamage;
		context.remainingDamage = std::max(0.f, damageBeforeArmor * (1.f - effectiveArmor));
		context.mitigatedDamage += damageBeforeArmor - context.remainingDamage;
		context.modifiedDamage = context.remainingDamage;

		const List<GameplayTag> appliedStatuses =
			DamageTypeSystem::ApplyStatusEffects(
				mAbilitySystemComponent,
				context
			);
		if (auto* sourceCombatant = context.source ? dynamic_cast<Combatant*>(context.source) : nullptr)
		{
			for (const GameplayTag& status : appliedStatuses)
			{
				if (status != DamageStatusSchema::Ignite)
				{
					continue;
				}
				sas::AbilityEvent event;
				event.eventTag = AttachmentSchema::Event::SourceIgniteApplied;
				event.SetSource(context.source);
				event.SetTarget(context.target);
				event.magnitude = context.remainingDamage;
				event.SetContext(&context);
				sourceCombatant->GetAbilitySystemComponent().HandleGameplayEvent(event);
			}
		}
		onDamageProcessed.Broadcast(context);
	}

	void CombatRuntime::QueueEffectEvent(
		const sas::GameplayEffectBehaviorEvent& behaviorEvent,
		const DamageContext* context
	)
	{
		sas::AbilityEvent event;
		event.eventTag = behaviorEvent.eventTag;
		event.SetSource(context ? context->source : &mOwner);
		event.SetTarget(&mOwner);
		event.magnitude = behaviorEvent.magnitude;
		event.SetContext(context);
		mPendingEffectEvents.push_back(event);
	}

	void CombatRuntime::DispatchPendingEffectEvents()
	{
		List<sas::AbilityEvent> events = std::move(mPendingEffectEvents);
		mPendingEffectEvents.clear();
		for (const sas::AbilityEvent& event : events)
		{
			mAbilitySystemComponent.HandleGameplayEvent(event);
		}
	}

	void CombatRuntime::NotifyDamageResolved(const DamageContext& context)
	{
		if (context.appliedDamage <= 0.f)
		{
			return;
		}

		sas::AbilityEvent event;
		event.eventTag = GameplayTag{ "Event.Owner.DamageTaken" };
		event.SetSource(context.source);
		event.SetTarget(context.target);
		event.magnitude = context.appliedDamage;
		event.SetContext(&context);
		mAbilitySystemComponent.HandleGameplayEvent(event);

		if (auto* sourceCombatant = context.source ? dynamic_cast<Combatant*>(context.source) : nullptr)
		{
			sas::AbilityEvent sourceEvent;
			sourceEvent.eventTag = AttachmentSchema::Event::SourceDamageDealt;
			sourceEvent.SetSource(context.source);
			sourceEvent.SetTarget(context.target);
			sourceEvent.magnitude = context.appliedDamage;
			sourceEvent.SetContext(&context);
			sourceCombatant->GetAbilitySystemComponent().HandleGameplayEvent(sourceEvent);
		}
		onDamageResolved.Broadcast(context);
	}
}
