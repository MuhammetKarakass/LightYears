#include "gameplay/module/ModuleShieldReflectionRuntime.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/targeting/TargetRelation.h"
#include "spaceShip/SpaceShip.h"
#include "framework/World.h"
#include <algorithm>
#include <limits>

namespace vega
{
	ModuleShieldReflectionRuntime::ModuleShieldReflectionRuntime(CombatRuntime& combat) : mCombat{ combat }, mShip{ dynamic_cast<SpaceShip*>(&combat.GetAbilitySystemComponent().GetOwner()) }
	{
		mEventHandle = mCombat.GetAbilitySystemComponent().onGameplayEvent.BindAction(this, &ModuleShieldReflectionRuntime::OnEvent);
		try { mDamageHandle = mCombat.onDamageResolved.BindAction(this, &ModuleShieldReflectionRuntime::OnDamage); }
		catch (...) { mCombat.GetAbilitySystemComponent().onGameplayEvent.UnbindAction(mEventHandle); throw; }
	}
	ModuleShieldReflectionRuntime::~ModuleShieldReflectionRuntime()
	{
		mCombat.GetAbilitySystemComponent().onGameplayEvent.UnbindAction(mEventHandle);
		mCombat.onDamageResolved.UnbindAction(mDamageHandle);
	}
	void ModuleShieldReflectionRuntime::Replace(const List<ResolvedModuleEffects::ShieldReflection>& rules)
	{
		++mGeneration;
		List<State> desired;
		for (const auto& rule : rules)
		{
			State next{ rule };
			for (const auto& previous : mRules)
				if (previous.resolved.instanceId == rule.instanceId && previous.resolved.ruleIndex == rule.ruleIndex &&
					previous.resolved.rule == rule.rule) next.remaining = previous.remaining;
			desired.push_back(next);
		}
		mRules = std::move(desired);
	}
	void ModuleShieldReflectionRuntime::Tick(float deltaSeconds)
	{
		if (deltaSeconds <= 0.f) return;
		for (auto& rule : mRules) rule.remaining = std::max(0.f, rule.remaining - deltaSeconds);
	}
	void ModuleShieldReflectionRuntime::OnEvent(const sas::AbilityEvent& event)
	{
		if (event.eventTag != GameplayTags::Event::Ability::Activated || !mShip) return;
		const auto* ability = mCombat.GetAbilitySystemComponent().GetAbilityById(event.sourceAbilityId.ToString());
		if (!ability || !sas::IsLoadoutAbilitySlot(ability->GetDefinition().slot) ||
			!ModuleTriggerEventTagsMatch({ GameplayTags::Ability::Defense }, event.sourceAbilityTags)) return;
		for (auto& rule : mRules) rule.remaining = rule.resolved.rule.durationSeconds;
	}
	void ModuleShieldReflectionRuntime::OnDamage(const DamageContext& context)
	{
		if (!mShip || !mShip->GetWorld() || mShip->GetIsPendingDestroy() || context.payload.skipModuleDamageRules || context.payload.moduleTriggerPolicy == DamageModuleTriggerPolicy::Suppressed || context.shieldDamage <= 0.f || context.target != mShip) return;
		const auto ownerLifetime = mShip->GetWeakActor().lock();
		if (!ownerLifetime) return;
		const auto generation = mGeneration;
		const auto snapshot = mRules;
		for (const auto& state : snapshot)
		{
			if (mGeneration != generation || mShip->GetIsPendingDestroy()) break;
			if (state.remaining <= 0.f) continue;
			shared_ptr<Actor> nearest;
			float nearestDistance = std::numeric_limits<float>::max();
			for (const auto& weak : mShip->GetWorld()->GetActorsByType<Actor>())
			{
				const auto target = weak.lock();
				if (!target || target->GetIsPendingDestroy() || !targeting::IsOpposingTarget(*mShip, *target) || !dynamic_cast<Combatant*>(target.get())) continue;
				const auto* health = target->FindComponent<HealthComponent>();
				if (health && health->GetHealth() <= 0.f) continue;
				const auto offset = target->GetActorLocation() - mShip->GetActorLocation();
				const float distance = offset.x * offset.x + offset.y * offset.y;
				if (distance < nearestDistance || (distance == nearestDistance && nearest && target->GetUniqueID() < nearest->GetUniqueID())) { nearest = target; nearestDistance = distance; }
			}
			if (!nearest) continue;
			auto payload = DamageTypeSystem::BuildPayload({ state.resolved.rule.damageType });
			payload.criticalPolicy = DamageCriticalPolicy::Disabled;
			payload.kineticStacks = 0;
			payload.electricStacks = state.resolved.rule.electricStacks;
			payload.skipModuleDamageRules = true;
			payload.moduleTriggerPolicy = DamageModuleTriggerPolicy::Suppressed;
			ApplyCombatDamage(*nearest, context.shieldDamage * state.resolved.rule.fraction, mShip, { state.resolved.rule.damageType }, payload);
		}
	}
}
