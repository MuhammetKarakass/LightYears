#include "gameplay/module/generated/ModuleTargetSlowRuntime.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/damage/DamageContext.h"
#include "gameplay/HealthComponent.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "spaceShip/SpaceShip.h"
#include <algorithm>

namespace ly
{
	namespace
	{
		bool SameActor(const weak_ptr<Actor>& target, const Actor* actor)
		{
			const auto locked = target.lock();
			return locked && locked.get() == actor;
		}
	}

	ModuleTargetSlowRuntime::ModuleTargetSlowRuntime(LightYearsAbilitySystemComponent& abilities) : mAbilities{ abilities }, mShip{ dynamic_cast<SpaceShip*>(&abilities.GetOwner()) }
	{
		mEventHandle = mAbilities.onGameplayEvent.BindAction(this, &ModuleTargetSlowRuntime::OnEvent);
	}

	ModuleTargetSlowRuntime::~ModuleTargetSlowRuntime()
	{
		mShuttingDown = true;
		mAbilities.onGameplayEvent.UnbindAction(mEventHandle);
		for (const Slowed& slowed : mSlowed) RemoveEffect(slowed);
		mSlowed.clear();
	}

	void ModuleTargetSlowRuntime::Replace(const List<ResolvedModuleTargetSlowRule>& rules)
	{
		if (mShuttingDown) return;
		mRules = rules;
		// Effects whose module instance left the ship end now; the rest run to their own expiry.
		for (auto tracked = mSlowed.begin(); tracked != mSlowed.end();)
		{
			const bool installed = std::any_of(mRules.begin(), mRules.end(),
				[&tracked](const ResolvedModuleTargetSlowRule& rule) { return rule.instanceId == tracked->instanceId; });
			if (installed)
			{
				++tracked;
				continue;
			}
			RemoveEffect(*tracked);
			tracked = mSlowed.erase(tracked);
		}
	}

	void ModuleTargetSlowRuntime::RemoveEffect(const Slowed& slowed)
	{
		if (!slowed.handle.IsValid()) return;
		if (const auto locked = slowed.target.lock())
			if (auto* combatant = dynamic_cast<Combatant*>(locked.get()))
				combatant->GetAbilitySystemComponent().RemoveGameplayEffect(slowed.handle);
	}

	void ModuleTargetSlowRuntime::OnEvent(const sas::AbilityEvent& event)
	{
		if (mShuttingDown || !mShip || mRules.empty() || event.eventTag != GameplayTags::Event::Source::DamageDealt) return;
		const auto* context = event.GetContext<DamageContext>();
		if (!context || context->source != mShip || context->appliedDamage <= 0.f || context->payload.skipModuleDamageRules ||
			context->payload.moduleTriggerPolicy == DamageModuleTriggerPolicy::Suppressed || !context->target) return;
		auto* targetCombatant = dynamic_cast<Combatant*>(context->target);
		if (!targetCombatant) return;

		const HealthComponent* ownerHealth = mShip->FindComponent<HealthComponent>();
		const float ownerMaxHealth = ownerHealth ? ownerHealth->GetMaxHealth() : 0.f;
		if (ownerMaxHealth <= 0.f) return;
		const float targetMaxHealth = context->targetMaxHealthAtResolution;
		const auto* targetShip = dynamic_cast<const SpaceShip*>(context->target);
		const bool bossClass = targetShip && targetShip->GetControlTargetClass() == ControlTargetClass::Boss;

		// Strongest matching rule wins; the duration is the longest of the matching rules.
		float slowFraction = 0.f;
		float seconds = 0.f;
		std::uint32_t instanceId = 0;
		for (const ResolvedModuleTargetSlowRule& resolved : mRules)
		{
			if (targetMaxHealth < resolved.rule.minRatio * ownerMaxHealth) continue;
			const float fraction = bossClass ? resolved.rule.bossSlowFraction : resolved.rule.slowFraction;
			if (fraction > slowFraction) { slowFraction = fraction; instanceId = resolved.instanceId; }
			seconds = std::max(seconds, resolved.rule.seconds);
		}
		if (slowFraction <= 0.f || seconds <= 0.f) return;

		auto& targetAbilities = targetCombatant->GetAbilitySystemComponent();
		const auto tracked = std::find_if(mSlowed.begin(), mSlowed.end(), [context](const Slowed& slowed) { return SameActor(slowed.target, context->target); });
		if (tracked != mSlowed.end() && tracked->handle.IsValid() && targetAbilities.FindGameplayEffect(tracked->handle))
		{
			tracked->instanceId = instanceId;
			targetAbilities.RefreshGameplayEffectDuration(tracked->handle);
			return;
		}

		const sas::GameplayEffectDefinition* slowDefinition = EffectData::FindGameplayEffectDefinition(MovementEffectSchema::SlowEffectId);
		if (!slowDefinition) return;
		sas::GameplayEffectSpec spec = sas::MakeGameplayEffectSpec(*slowDefinition);
		spec.duration = seconds;
		spec.maxStacks = 1;
		spec.modifiers = { sas::AttributeModifier{ OwnerAttributeIds::MovementSlow, sas::AttributeModifierOperation::Add, slowFraction } };
		const sas::GameplayEffectHandle handle = targetAbilities.ApplyGameplayEffect(spec, sas::GameplayEffectSourceContext{ mShip });
		if (!handle.IsValid()) return;
		if (tracked != mSlowed.end())
		{
			tracked->instanceId = instanceId;
			tracked->handle = handle;
		}
		else
		{
			mSlowed.push_back(Slowed{ instanceId, weak_ptr<Actor>{ context->target->GetWeakActor() }, handle });
		}
	}
}
