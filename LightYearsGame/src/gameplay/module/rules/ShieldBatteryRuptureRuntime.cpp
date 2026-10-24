#include "gameplay/module/rules/ShieldBatteryRuptureRuntime.h"

#include "framework/GameplayTag.h"
#include "framework/World.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/damage/DamageContext.h"
#include "gameplay/module/generated/ModuleDelayedExplosionActor.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/moduleGenerated/ModuleGeneratedPresentationProfile.h"
#include "spaceShip/SpaceShip.h"

#include <algorithm>

namespace vega
{
	void ApplyShieldBatteryBehavior(const List<ResolvedModuleShieldBatteryRule>& rules, ShieldModuleBehavior& behavior)
	{
		for (const ResolvedModuleShieldBatteryRule& resolved : rules)
		{
			behavior.batteryCapFraction = std::max(behavior.batteryCapFraction, resolved.rule.reserveFraction);
			behavior.batteryDamagedFillFraction = std::max(behavior.batteryDamagedFillFraction, resolved.rule.damagedFillFraction);
		}
	}

	ModuleShieldRuptureRuntime::ModuleShieldRuptureRuntime(LightYearsAbilitySystemComponent& abilities) : mAbilities{ abilities }, mShip{ dynamic_cast<SpaceShip*>(&abilities.GetOwner()) }
	{
		mEventHandle = mAbilities.onGameplayEvent.BindAction(this, &ModuleShieldRuptureRuntime::OnEvent);
	}

	ModuleShieldRuptureRuntime::~ModuleShieldRuptureRuntime()
	{
		mShuttingDown = true;
		mAbilities.onGameplayEvent.UnbindAction(mEventHandle);
		for (auto& state : mStates) Cancel(state.lease);
	}

	void ModuleShieldRuptureRuntime::Cancel(const std::shared_ptr<ModuleGeneratedLease>& lease) noexcept
	{
		if (!lease) return;
		lease->active = false;
		const auto actors = std::move(lease->actors);
		lease->actors.clear();
		for (const auto& weak : actors) if (const auto actor = weak.lock()) { try { actor->Destroy(); } catch (...) {} }
	}

	void ModuleShieldRuptureRuntime::Prune(const std::shared_ptr<ModuleGeneratedLease>& lease)
	{
		auto& actors = lease->actors;
		actors.erase(std::remove_if(actors.begin(), actors.end(), [](const auto& weak) { const auto actor = weak.lock(); return !actor || actor->GetIsPendingDestroy(); }), actors.end());
	}

	// Unchanged rules keep their lease, so a reapply does not cancel an explosion already in flight.
	void ModuleShieldRuptureRuntime::Replace(const List<ResolvedModuleShieldRuptureRule>& rules)
	{
		if (mShuttingDown) return;
		List<State> next;
		for (const ResolvedModuleShieldRuptureRule& resolved : rules)
		{
			State state;
			state.instanceId = resolved.instanceId;
			state.ruleIndex = resolved.ruleIndex;
			state.rule = resolved.rule;
			const auto previous = std::find_if(mStates.begin(), mStates.end(), [&state](const State& value)
			{
				return value.instanceId == state.instanceId && value.ruleIndex == state.ruleIndex && value.rule == state.rule && value.lease;
			});
			if (previous != mStates.end())
			{
				state.lease = previous->lease;
				previous->lease.reset();
			}
			else
			{
				state.lease = std::make_shared<ModuleGeneratedLease>();
				state.lease->key = mNextKey++;
			}
			next.push_back(std::move(state));
		}
		for (auto& previous : mStates) Cancel(previous.lease);
		mStates = std::move(next);
	}

	void ModuleShieldRuptureRuntime::Tick(float deltaSeconds)
	{
		if (mShuttingDown || deltaSeconds <= 0.f) return;
		for (ShieldLoss& loss : mLosses) loss.remainingSeconds -= deltaSeconds;
		mLosses.erase(std::remove_if(mLosses.begin(), mLosses.end(), [](const ShieldLoss& loss) { return loss.remainingSeconds <= 0.f; }), mLosses.end());
		for (auto& state : mStates) if (state.lease) Prune(state.lease);
	}

	std::size_t ModuleShieldRuptureRuntime::GetActiveActorCount() const
	{
		std::size_t count = 0;
		for (const State& state : mStates)
			if (state.lease) for (const auto& weak : state.lease->actors) if (const auto actor = weak.lock(); actor && !actor->GetIsPendingDestroy()) ++count;
		return count;
	}

	void ModuleShieldRuptureRuntime::OnEvent(const sas::AbilityEvent& event)
	{
		if (mShuttingDown || !mShip || mStates.empty() || event.eventTag != GameplayTags::Event::Source::DamageDealt) return;
		const auto* context = event.GetContext<DamageContext>();
		if (!context || context->source != mShip || context->appliedDamage <= 0.f || context->shieldDamage <= 0.f || !context->targetWasEnemyCombatant) return;
		// Module-generated damage (including Rupture's own explosion) never feeds the history or detonates again.
		if (context->payload.skipModuleDamageRules || context->payload.moduleTriggerPolicy == DamageModuleTriggerPolicy::Suppressed) return;
		Actor* target = context->target;
		const auto* shield = target ? target->FindComponent<ShieldComponent>() : nullptr;
		if (!shield) return;

		const std::uint64_t targetId = target->GetUniqueID();
		float window = 0.f;
		for (const State& state : mStates) window = std::max(window, state.rule.historySeconds);
		mLosses.push_back({ targetId, context->shieldDamage, window });
		if (shield->GetShield() > 0.f) return;

		// The hit broke the shield: the loss window of this target, including this hit, sizes the explosion and is spent.
		float lost = 0.f;
		for (const ShieldLoss& loss : mLosses) if (loss.targetId == targetId) lost += loss.amount;
		mLosses.erase(std::remove_if(mLosses.begin(), mLosses.end(), [targetId](const ShieldLoss& loss) { return loss.targetId == targetId; }), mLosses.end());
		const sf::Vector2f origin{ context->targetLocationAtResolution.x, context->targetLocationAtResolution.y };
		for (const State& state : mStates) Detonate(state, origin, lost);
	}

	void ModuleShieldRuptureRuntime::Detonate(const State& state, const sf::Vector2f& origin, float lostShield)
	{
		if (!state.lease || !state.lease->active || !mShip || !mShip->GetWorld() || mShip->GetIsPendingDestroy()) return;
		const float damage = lostShield * state.rule.damageFraction;
		const auto* profile = PresentationProfileRegistry<ModuleExplosionPresentationProfile>::Find(ModuleGeneratedPresentationIds::Explosion.ToString());
		if (damage <= 0.f || state.rule.radius <= 0.f || !profile) return;
		Prune(state.lease);
		const auto actor = mShip->GetWorld()->SpawnActor<ModuleDelayedExplosionActor>(mShip->GetWeakActor(), *profile, state.lease, damage, state.rule.radius, 0.f, List<GameplayTag>{ DamageTypeSchema::Energy }).lock();
		if (actor)
		{
			actor->SetActorLocation(origin);
			state.lease->actors.push_back(actor);
		}
	}
}
