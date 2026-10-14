#include "gameplay/module/generated/ModuleOwnerBurstRuntime.h"
#include "gameplay/module/generated/ModuleGeneratedDamage.h"
#include "effects/GameplayEffectSpec.h"
#include "framework/Actor.h"
#include "framework/World.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "gameplay/ShieldComponent.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/control/ControlResponse.h"
#include "gameplay/damage/DamageContext.h"
#include "gameplay/movement/MovementInfluenceService.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameplay/targeting/CombatantTargetQuery.h"
#include "spaceShip/SpaceShip.h"
#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		// The catalog stun shared with Blastback and NullPulse. Its Stunned tag blocks movement and abilities, and it
		// adds no status, so status-counting rules see nothing from a burst.
		constexpr char kStunEffectId[] = "Effect.Control.Stun.Basic";
		// Impulse retention used when the target is not a ship; ships use their linear damping, as Blastback does.
		constexpr float kDefaultPushRetention = 0.01f;

		bool IsShipAlive(SpaceShip* ship)
		{
			return ship && ship->GetWorld() && !ship->GetIsPendingDestroy() && ship->GetHealthComponent().GetHealth() > 0.f;
		}

		bool IsListed(Actor& target, const List<ControlTargetClass>& classes)
		{
			const auto* ship = dynamic_cast<const SpaceShip*>(&target);
			const ControlTargetClass targetClass = ship ? ship->GetControlTargetClass() : ControlTargetClass::Normal;
			return std::find(classes.begin(), classes.end(), targetClass) != classes.end();
		}

		// A negative impulse pulls the target toward the origin.
		void PushAway(Actor& target, const sf::Vector2f& origin, float impulse)
		{
			sf::Vector2f direction = target.GetActorLocation() - origin;
			const float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
			direction = length > 0.001f ? direction / length : sf::Vector2f{ 1.f, 0.f };
			float retention = kDefaultPushRetention;
			if (auto* ship = dynamic_cast<SpaceShip*>(&target)) retention = ship->GetMovementComponent().GetAttributes().linearDamping.currentValue;
			movement::MovementInfluenceService::ApplyImpulse(target, movement::ImpulseRequest{ direction * impulse, retention });
		}

		void Deactivate(const std::shared_ptr<ModuleGeneratedLease>& lease) noexcept
		{
			if (lease) lease->active = false;
		}
	}

	ModuleOwnerBurstRuntime::ModuleOwnerBurstRuntime(LightYearsAbilitySystemComponent& abilities)
		: mAbilities{ abilities }, mShip{ dynamic_cast<SpaceShip*>(&abilities.GetOwner()) }
	{
		if (mShip) mShieldHandle = mShip->GetShieldComponent().onShieldDamaged.BindAction(this, &ModuleOwnerBurstRuntime::OnShieldDamaged);
		mEventHandle = mAbilities.onGameplayEvent.BindAction(this, &ModuleOwnerBurstRuntime::OnGameplayEvent);
	}

	ModuleOwnerBurstRuntime::~ModuleOwnerBurstRuntime()
	{
		mShuttingDown = true;
		if (mShip) mShip->GetShieldComponent().onShieldDamaged.UnbindAction(mShieldHandle);
		mAbilities.onGameplayEvent.UnbindAction(mEventHandle);
		for (State& state : mStates) Deactivate(state.lease);
		for (DelayedWave& wave : mDelayedWaves) Deactivate(wave.lease);
		mStates.clear();
		mDelayedWaves.clear();
	}

	void ModuleOwnerBurstRuntime::Replace(const List<ResolvedModuleOwnerBurstRule>& rules)
	{
		if (!mShuttingDown) Reconcile(rules);
	}

	void ModuleOwnerBurstRuntime::Reconcile(const List<ResolvedModuleOwnerBurstRule>& rules)
	{
		List<State> next;
		for (const ResolvedModuleOwnerBurstRule& resolved : rules)
		{
			State state;
			state.instanceId = resolved.instanceId;
			state.rule = resolved.rule;
			state.accumulatedArmor = resolved.accumulatedArmor;
			const auto previous = std::find_if(mStates.begin(), mStates.end(), [&state](const State& value)
			{
				return value.instanceId == state.instanceId && value.rule == state.rule;
			});
			if (previous != mStates.end())
			{
				state.lease = previous->lease;
				state.cooldownRemaining = previous->cooldownRemaining;
				state.charges = previous->charges;
				mStates.erase(previous);
			}
			else
			{
				state.lease = std::make_shared<ModuleGeneratedLease>();
			}
			next.push_back(std::move(state));
		}
		for (State& previous : mStates) Deactivate(previous.lease);
		mStates = std::move(next);
	}

	void ModuleOwnerBurstRuntime::OnShieldDamaged(float absorbed, float remainingShield, float maxShield)
	{
		(void)maxShield;
		// A break caused by module damage does not fire. This is the same exclusion the ShieldBroken buff path applies.
		if (mShuttingDown || !mShip || absorbed <= 0.f || remainingShield != 0.f || mShip->GetShieldComponent().IsAbsorbingModuleDamage()) return;
		if (mStates.empty()) return;
		++mPendingBreaks;
		Flush();
	}

	// Only kills the owner itself made count, and never module-generated ones: a burst's own kills do not charge it.
	void ModuleOwnerBurstRuntime::OnGameplayEvent(const sas::AbilityEvent& event)
	{
		if (mShuttingDown || !mShip) return;
		if (event.eventTag == GameplayTags::Event::Ability::Activated)
		{
			const auto* ability = mAbilities.GetAbilityById(event.sourceAbilityId.ToString());
			if (!ability || !sas::IsLoadoutAbilitySlot(ability->GetDefinition().slot) || !IsShipAlive(mShip)) return;
			for (auto& state : mStates)
				if (state.rule.trigger == ModuleTriggerKind::AbilityActivated && state.cooldownRemaining <= 0.f && ModuleTriggerEventTagsMatch(state.rule.requiredEventTags, ability->GetDefinition().abilityTags))
				{ state.cooldownRemaining = state.rule.cooldownSeconds; state.detonating = true; }
			Flush(); return;
		}
		if (event.eventTag != GameplayTags::Event::Combat::KillConfirmed) return;
		const DamageContext* damage = event.GetContext<DamageContext>();
		if (!damage || damage->payload.skipModuleDamageRules || damage->payload.moduleTriggerPolicy == DamageModuleTriggerPolicy::Suppressed ||
			damage->source != &mAbilities.GetOwner() || !damage->targetWasKilled || !damage->targetWasEnemyCombatant) return;
		if (mStates.empty() || !IsShipAlive(mShip)) return;
		const sf::Vector2f origin = mShip->GetActorLocation();
		const sf::Vector2f offset{ damage->targetLocationAtResolution.x - origin.x, damage->targetLocationAtResolution.y - origin.y };
		const float distance = std::sqrt(offset.x * offset.x + offset.y * offset.y);
		bool charged = false;
		for (State& state : mStates)
		{
			if (state.rule.trigger != ModuleTriggerKind::KillConfirmed || distance > state.rule.chargeRadius) continue;
			state.charges = std::min(state.charges + 1, state.rule.chargeKills);
			charged = true;
		}
		if (!charged) return;
		ArmCharged();
		Flush();
	}

	void ModuleOwnerBurstRuntime::ArmCharged()
	{
		for (State& state : mStates)
		{
			if (state.rule.trigger != ModuleTriggerKind::KillConfirmed || state.charges < state.rule.chargeKills || state.cooldownRemaining > 0.f) continue;
			state.charges = 0;
			state.cooldownRemaining = state.rule.cooldownSeconds;
			state.detonating = true;
		}
	}

	void ModuleOwnerBurstRuntime::Flush()
	{
		if (mBusy || mShuttingDown) return;
		mBusy = true;
		try
		{
			for (;;)
			{
				if (mPendingBreaks > 0)
				{
					--mPendingBreaks;
					BurstShield();
					continue;
				}
				const auto ready = std::find_if(mStates.begin(), mStates.end(), [](const State& state) { return state.detonating; });
				if (ready == mStates.end()) break;
				ready->detonating = false;
				// Copy before detonating: a callback may reconcile mStates.
				const ModuleOwnerBurstRule rule = ready->rule;
				const std::shared_ptr<ModuleGeneratedLease> lease = ready->lease;
				const float armor = ready->accumulatedArmor;
				Detonate(rule, lease, armor);
			}
		}
		catch (...) { mBusy = false; throw; }
		mBusy = false;
	}

	void ModuleOwnerBurstRuntime::BurstShield()
	{
		if (!IsShipAlive(mShip)) return;
		for (std::size_t index = 0; index < mStates.size(); ++index)
		{
			if (mStates[index].rule.trigger != ModuleTriggerKind::ShieldBroken || mStates[index].cooldownRemaining > 0.f) continue;
			mStates[index].cooldownRemaining = mStates[index].rule.cooldownSeconds;
			const ModuleOwnerBurstRule rule = mStates[index].rule;
			const std::shared_ptr<ModuleGeneratedLease> lease = mStates[index].lease;
			Detonate(rule, lease);
		}
	}

	// One detonation of a rule. With a pull, enemies are drawn in now and the wave lands pullLeadSeconds later; echoes
	// follow the detonation by their own delay. Echo damage is echoFraction of the first wave and its push is echoPushFraction.
	void ModuleOwnerBurstRuntime::Detonate(const ModuleOwnerBurstRule& rule, const std::shared_ptr<ModuleGeneratedLease>& lease, float accumulatedArmor)
	{
		if (!lease || !lease->active || !IsShipAlive(mShip)) return;
		const sf::Vector2f origin = mShip->GetActorLocation();
		const float damage = rule.accumulatedArmorCoefficient * accumulatedArmor + (rule.damageFractionOfMaxHealth > 0.f ? mShip->GetHealthComponent().GetMaxHealth() * rule.damageFractionOfMaxHealth : rule.damage) + rule.energyPowerCoefficient * std::max(0.f, mAbilities.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower));
		float lead = 0.f;
		if (rule.pullImpulse > 0.f && rule.pullLeadSeconds > 0.f)
		{
			Pull(rule, lease, origin);
			lead = rule.pullLeadSeconds;
			ScheduleWave(rule, lease, origin, damage, 1.f, lead);
		}
		else
		{
			Wave(rule, lease, origin, damage, 1.f);
		}
		if (rule.echoDelaySeconds > 0.f) ScheduleWave(rule, lease, origin, damage * rule.echoFraction, rule.echoPushFraction, lead + rule.echoDelaySeconds);
	}

	void ModuleOwnerBurstRuntime::Pull(const ModuleOwnerBurstRule& rule, const std::shared_ptr<ModuleGeneratedLease>& lease, const sf::Vector2f& origin)
	{
		const auto targets = targeting::FindOpposingCombatants(*mShip->GetWorld(), *mShip, origin, rule.radius, false);
		for (const auto& target : targets)
		{
			if (!lease->active || !IsShipAlive(mShip)) break;
			if (!target || target->GetIsPendingDestroy() || IsListed(*target, rule.pullImmuneClasses)) continue;
			PushAway(*target, origin, -rule.pullImpulse);
		}
	}

	void ModuleOwnerBurstRuntime::Wave(const ModuleOwnerBurstRule& rule, const std::shared_ptr<ModuleGeneratedLease>& lease, const sf::Vector2f& origin, float damage, float pushScale)
	{
		if (!lease || !lease->active || !IsShipAlive(mShip)) return;
		// Damage goes first: a push before it could move an enemy out of the radius and skip its damage.
		if (damage > 0.f) ApplyModuleGeneratedAreaDamage(*mShip, *mShip, lease, origin, rule.radius, damage, { rule.damageType }, 0, DamageSourceScalingPolicy::AlreadyResolved, rule.igniteStacks);
		const bool push = rule.pushImpulse > 0.f && pushScale > 0.f;
		if (!push && rule.stunSeconds <= 0.f) return;
		// Re-query after damage: dead targets are pending destroy and are skipped below.
		const auto targets = targeting::FindOpposingCombatants(*mShip->GetWorld(), *mShip, origin, rule.radius, false);
		for (const auto& target : targets)
		{
			if (!lease->active || !IsShipAlive(mShip)) break;
			if (!target || target->GetIsPendingDestroy()) continue;
			if (push) PushAway(*target, origin, rule.pushImpulse * pushScale);
			if (rule.stunSeconds > 0.f && !IsListed(*target, rule.stunImmuneClasses)) Stun(*target, rule.stunSeconds);
		}
	}

	void ModuleOwnerBurstRuntime::ScheduleWave(const ModuleOwnerBurstRule& rule, const std::shared_ptr<ModuleGeneratedLease>& lease, const sf::Vector2f& origin, float damage, float pushScale, float seconds)
	{
		DelayedWave wave;
		wave.lease = lease;
		wave.rule = rule;
		wave.damage = damage;
		wave.pushScale = pushScale;
		wave.origin = origin;
		wave.remainingSeconds = seconds;
		mDelayedWaves.push_back(std::move(wave));
	}

	void ModuleOwnerBurstRuntime::Stun(Actor& target, float seconds)
	{
		auto* combatant = dynamic_cast<Combatant*>(&target);
		const sas::GameplayEffectDefinition* definition = EffectData::FindGameplayEffectDefinition(kStunEffectId);
		if (!combatant || !definition) return;
		sas::GameplayEffectSpec spec = sas::MakeGameplayEffectSpec(*definition);
		spec.duration = seconds;
		spec.maxStacks = 1;
		combatant->GetAbilitySystemComponent().ApplyGameplayEffect(spec, sas::GameplayEffectSourceContext{ mShip });
	}

	void ModuleOwnerBurstRuntime::Tick(float deltaSeconds)
	{
		if (mShuttingDown || mBusy || deltaSeconds <= 0.f) return;
		for (State& state : mStates) state.cooldownRemaining = std::max(0.f, state.cooldownRemaining - deltaSeconds);
		mBusy = true;
		try
		{
			for (std::size_t index = 0; index < mDelayedWaves.size();)
			{
				if (!mDelayedWaves[index].lease || !mDelayedWaves[index].lease->active)
				{
					mDelayedWaves.erase(mDelayedWaves.begin() + static_cast<std::ptrdiff_t>(index));
					continue;
				}
				mDelayedWaves[index].remainingSeconds -= deltaSeconds;
				if (mDelayedWaves[index].remainingSeconds > 0.f)
				{
					++index;
					continue;
				}
				const DelayedWave wave = mDelayedWaves[index];
				mDelayedWaves.erase(mDelayedWaves.begin() + static_cast<std::ptrdiff_t>(index));
				Wave(wave.rule, wave.lease, wave.origin, wave.damage, wave.pushScale);
			}
		}
		catch (...) { mBusy = false; throw; }
		mBusy = false;
		// A cooldown that ended may release a charged rule.
		ArmCharged();
		Flush();
	}

	void ModuleOwnerBurstRuntime::PushOwnerCentred(SpaceShip& owner, float radius, float impulse)
	{
		if (!IsShipAlive(&owner) || !(radius > 0.f) || !(impulse > 0.f)) return;
		const sf::Vector2f origin = owner.GetActorLocation();
		const auto targets = targeting::FindOpposingCombatants(*owner.GetWorld(), owner, origin, radius, false);
		for (const auto& target : targets)
		{
			if (!target || target->GetIsPendingDestroy()) continue;
			PushAway(*target, origin, impulse);
		}
	}

	std::size_t ModuleOwnerBurstRuntime::GetRuleCount() const { return mStates.size(); }
	std::size_t ModuleOwnerBurstRuntime::GetPendingEchoCount() const { return mDelayedWaves.size(); }
}
