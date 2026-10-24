#include "gameplay/module/ModuleReactiveRuntime.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/attributes/AttributeIds.h"
#include "spaceShip/SpaceShip.h"
#include "gameplay/targeting/CombatantTargetQuery.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/combat/Combatant.h"
#include "gameConfigs/combat/WeaponStructs.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/module/generated/ModuleOwnerBurstRuntime.h"
#include "gameplay/module/generated/ModuleSmolderingFieldActor.h"
#include "framework/World.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/moduleGenerated/ModuleGeneratedPresentationProfile.h"
#include "gameplay/time/PeriodicTickAccumulator.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace vega
{
	namespace
	{
		std::string TemporaryShieldSource(std::uint32_t instanceId, std::size_t index) { return "Module.TemporaryShield." + std::to_string(instanceId) + "." + std::to_string(index); }
		std::string ConditionalMovementSource(std::uint32_t instanceId, std::size_t index) { return "Module.ConditionalMovement." + std::to_string(instanceId) + "." + std::to_string(index); }
	}
	ModuleReactiveRuntime::ModuleReactiveRuntime(LightYearsAbilitySystemComponent& abilities) : mAbilities{ abilities }, mShip{ dynamic_cast<SpaceShip*>(&abilities.GetOwner()) }
	{
		try
		{
			mEventHandle = mAbilities.onGameplayEvent.BindAction(this, &ModuleReactiveRuntime::OnEvent);
			mAttributeHandle = mAbilities.GetAttributes().onAttributeChanged.BindAction(this, &ModuleReactiveRuntime::OnAttribute);
			if (mShip)
			{
				mHealthHandle = mShip->GetHealthComponent().onHealthChanged.BindAction(this, &ModuleReactiveRuntime::OnHealth);
				mShieldHandle = mShip->GetShieldComponent().onShieldChanged.BindAction(this, &ModuleReactiveRuntime::OnShield);
			}
		}
		catch (...)
		{
			mAbilities.onGameplayEvent.UnbindAction(mEventHandle);
			mAbilities.GetAttributes().onAttributeChanged.UnbindAction(mAttributeHandle);
			if (mShip) { mShip->GetHealthComponent().onHealthChanged.UnbindAction(mHealthHandle); mShip->GetShieldComponent().onShieldChanged.UnbindAction(mShieldHandle); }
			throw;
		}
	}
	ModuleReactiveRuntime::~ModuleReactiveRuntime()
	{
		mShuttingDown = true;
		mAbilities.onGameplayEvent.UnbindAction(mEventHandle);
		mAbilities.GetAttributes().onAttributeChanged.UnbindAction(mAttributeHandle);
		if (mShip) { mShip->GetHealthComponent().onHealthChanged.UnbindAction(mHealthHandle); mShip->GetShieldComponent().onShieldChanged.UnbindAction(mShieldHandle); }
		for (const auto& rule : mConditionalMovement) if (mShip) mShip->GetRuntimeModifiers().Remove(ConditionalMovementSource(rule.instanceId, rule.ruleIndex));
		for (const auto& state : mActions) if (mShip && state.resolved.rule.kind == ModuleEventActionKind::GrantTemporaryShield) mShip->GetShieldComponent().RemoveModuleTemporaryShield(TemporaryShieldSource(state.resolved.instanceId, state.resolved.ruleIndex));
		for (AttributeState& state : mAttributes) Remove(state);
		for (SustainedSpeedState& state : mSustainedSpeed) ClearSustained(state);
		CancelSmolderFields();
	}
	void ModuleReactiveRuntime::Replace(const List<ResolvedModuleEventAction>& actions, const List<ResolvedModuleOwnerAttributeRule>& attributes, const List<ResolvedModuleEffects::MovementFade>& fades)
	{
		if (mShuttingDown) return;
		CancelSmolderFields();
		mPending = DesiredRules{ actions, attributes, fades };
		Flush();
	}
	void ModuleReactiveRuntime::Tick(float deltaSeconds)
	{
		if (deltaSeconds < 0.f || mShuttingDown) return;
		float heal = 0.f;
		for (auto& state : mActions)
		{
			state.cooldownRemaining = std::max(0.f, state.cooldownRemaining - deltaSeconds);
			const float elapsed = std::min(deltaSeconds, state.healingRemaining);
			heal += elapsed * state.healingRate; state.healingRemaining -= elapsed;
		}
		if (heal > 0.f && mShip && mShip->GetHealthComponent().GetHealth() > 0.f) mShip->GetHealthComponent().Regenerate(heal);
		mDirty = true;
		TickSustainedSpeed(deltaSeconds);
		TickCharge(deltaSeconds);
		Flush(deltaSeconds);
	}

	void ModuleReactiveRuntime::ReplaceCharge(const List<ResolvedModuleChargeRule>& rules)
	{
		if (mShuttingDown) return;
		List<ChargeTrickleState> next;
		for (const ResolvedModuleChargeRule& rule : rules)
		{
			if (!(rule.trickleSeconds > 0.f) || rule.trickleCharges <= 0 || rule.slots.empty()) continue;
			ChargeTrickleState state;
			state.resolved = rule;
			for (const ChargeTrickleState& previous : mChargeTrickles)
			{
				if (previous.resolved.instanceId == rule.instanceId && previous.resolved.ruleIndex == rule.ruleIndex && previous.resolved.trickleSeconds == rule.trickleSeconds)
				{
					state.accumulator = previous.accumulator;
				}
			}
			next.push_back(std::move(state));
		}
		mChargeTrickles = std::move(next);
	}

	void ModuleReactiveRuntime::TickCharge(float deltaSeconds)
	{
		if (deltaSeconds <= 0.f || mShuttingDown) return;
		for (ChargeTrickleState& state : mChargeTrickles)
		{
			const int dueTicks = time::ConsumePeriodicTicks(state.accumulator, deltaSeconds, state.resolved.trickleSeconds);
			for (int tick = 0; tick < dueTicks; ++tick) GrantTrickle(state.resolved);
		}
	}

	// The adjacent ability with the fewest charges takes the grant (ties keep geometric order). A grant that does not fit spills to the
	// other adjacent abilities only when overflowTransfer is set. Capacity is never exceeded: the ASC grant only fills missing charges.
	void ModuleReactiveRuntime::GrantTrickle(const ResolvedModuleChargeRule& rule)
	{
		std::optional<sas::AbilitySlot> target;
		int fewest = std::numeric_limits<int>::max();
		for (const sas::AbilitySlot slot : rule.slots)
		{
			const GameAbility* ability = mAbilities.GetAbility(slot);
			if (!ability || ability->GetChargeCapacity() <= 0) continue;
			if (ability->GetCharges() < fewest)
			{
				fewest = ability->GetCharges();
				target = slot;
			}
		}
		if (!target) return;

		int remaining = rule.trickleCharges - mAbilities.GrantAbilityCharge(*target, rule.trickleCharges);
		if (!rule.overflowTransfer) return;
		for (const sas::AbilitySlot slot : rule.slots)
		{
			if (remaining <= 0) return;
			if (slot == *target) continue;
			remaining -= mAbilities.GrantAbilityCharge(slot, remaining);
		}
	}
	void ModuleReactiveRuntime::ReplaceSustainedSpeed(const List<ResolvedModuleEffects::SustainedSpeedRule>& rules)
	{
		if (mShuttingDown) return;
		List<SustainedSpeedState> next;
		next.reserve(rules.size());
		for (const auto& resolved : rules)
		{
			SustainedSpeedState state;
			state.resolved = resolved;
			for (auto& previous : mSustainedSpeed)
			{
				if (previous.resolved.instanceId != resolved.instanceId || previous.resolved.ruleIndex != resolved.ruleIndex || !(previous.resolved.rule == resolved.rule)) continue;
				state.fastSeconds = previous.fastSeconds;
				state.lingerSeconds = previous.lingerSeconds;
				state.lingering = previous.lingering;
				state.sonic = previous.sonic;
				state.damageRegistered = previous.damageRegistered;
				state.turnRegistered = previous.turnRegistered;
				state.primaryHandles = std::move(previous.primaryHandles);
				previous.primaryHandles.clear();
				previous.damageRegistered = false;
				previous.turnRegistered = false;
				break;
			}
			next.push_back(std::move(state));
		}
		// Whatever was not carried over (removed, rebooting or changed rules) loses its Primary modifiers now.
		for (SustainedSpeedState& previous : mSustainedSpeed) ClearSustained(previous);
		mSustainedSpeed = std::move(next);
	}
	std::size_t ModuleReactiveRuntime::GetOwnedSustainedHandleCount() const
	{
		std::size_t count = 0;
		for (const SustainedSpeedState& state : mSustainedSpeed) count += state.primaryHandles.size() + (state.damageRegistered ? 1u : 0u) + (state.turnRegistered ? 1u : 0u);
		return count;
	}
	void ModuleReactiveRuntime::ClearSustained(SustainedSpeedState& state) noexcept
	{
		for (std::size_t handle : state.primaryHandles)
		{
			try { mAbilities.RemovePrimaryWeaponModifier(handle); }
			catch (...) {}
		}
		state.primaryHandles.clear();
		if (state.damageRegistered)
		{
			try { SetSonicDamage(state, false); }
			catch (...) {}
		}
		if (state.turnRegistered)
		{
			try { SetSonicTurn(state, false); }
			catch (...) {}
		}
		state.fastSeconds = 0.f;
		state.lingerSeconds = 0.f;
		state.lingering = false;
		state.sonic = false;
	}
	// Primary modifiers are written only when the sonic state flips, so a steady state adds no modifier traffic.
	// Whole-shot Primary damage multiplier. It lives in the owner's CombatRuntime, which applies it to the Primary hit
	// after AttackPower scaling, so it cannot reach ability or periodic damage.
	void ModuleReactiveRuntime::SetSonicDamage(SustainedSpeedState& state, bool active)
	{
		if (!mShip) return;
		CombatRuntime& combat = mShip->GetCombatRuntime();
		const std::string source = "ModuleSustainedSpeed." + std::to_string(state.resolved.instanceId) + "." + std::to_string(state.resolved.ruleIndex);
		if (active && state.resolved.rule.primaryDamageMultiplier != 1.f)
		{
			combat.SetPrimaryShotModifier(source, state.resolved.rule.primaryDamageMultiplier);
			state.damageRegistered = true;
			return;
		}
		combat.RemovePrimaryShotModifier(source);
		state.damageRegistered = false;
	}
	void ModuleReactiveRuntime::SetSonic(SustainedSpeedState& state, bool active)
	{
		if (state.sonic == active) return;
		state.sonic = active;
		for (std::size_t handle : state.primaryHandles) mAbilities.RemovePrimaryWeaponModifier(handle);
		state.primaryHandles.clear();
		if (!active)
		{
			SetSonicDamage(state, false);
			SetSonicTurn(state, false);
			return;
		}
		const ModuleSustainedSpeedRule& rule = state.resolved.rule;
		if (rule.primarySpeedMultiplier != 1.f)
			state.primaryHandles.push_back(mAbilities.AddPrimaryWeaponModifier(sas::AttributeModifier{ PrimaryWeaponSchema::Projectile::Delivery::Speed, sas::AttributeModifierOperation::Multiply, rule.primarySpeedMultiplier }));
		if (rule.primaryPierceBonus > 0.f)
			state.primaryHandles.push_back(mAbilities.AddPrimaryWeaponModifier(sas::AttributeModifier{ PrimaryWeaponSchema::Projectile::Delivery::PierceCount, sas::AttributeModifierOperation::Add, rule.primaryPierceBonus }));
		SetSonicDamage(state, true);
		SetSonicTurn(state, true);
	}
	void ModuleReactiveRuntime::SetSonicTurn(SustainedSpeedState& state, bool active)
	{
		if (!mShip) return;
		const std::string source = "ModuleSlipstream." + std::to_string(state.resolved.instanceId) + "." + std::to_string(state.resolved.ruleIndex);
		if (active && state.resolved.rule.turnCapabilityMultiplier > 1.f)
		{
			ShipRuntimeModifier modifier;
			modifier.turnCapabilityMultiplier = state.resolved.rule.turnCapabilityMultiplier;
			mShip->GetRuntimeModifiers().Set(source, modifier);
			state.turnRegistered = true;
		}
		else
		{
			mShip->GetRuntimeModifiers().Remove(source);
			state.turnRegistered = false;
		}
	}
	// Sonic = speed at or above speedFraction of the live cap for holdSeconds without a break. A break resets the hold
	// counter; once sonic, the state persists for lingerSeconds after the speed drops. Speed is the last movement step
	// (distance over time, so a ship pinned against geometry is not flying), and the cap is the movement tick's own cap.
	void ModuleReactiveRuntime::TickSustainedSpeed(float deltaSeconds)
	{
		if (!mShip || mSustainedSpeed.empty()) return;
		constexpr float kTimeEpsilon = 1e-4f;
		constexpr float kSpeedEpsilon = 0.01f;
		const bool alive = mShip->GetHealthComponent().GetHealth() > 0.f;
		const sf::Vector2f velocity = mShip->GetVelocity();
		const float length = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
		const sf::Vector2f direction = length > 0.001f ? velocity / length : sf::Vector2f{ 0.f, 0.f };
		const float capMultiplier = mShip->ResolveMovementSpeedCapMultiplier() * mShip->GetMovementSpeedMultiplier() * mShip->GetConditionalMovementSpeedMultiplier(direction);
		const float cap = mShip->GetMovementComponent().ResolveCurrentSpeedCap(capMultiplier);
		const float speed = mShip->GetMovementComponent().GetCurrentMovementSpeed();
		for (SustainedSpeedState& state : mSustainedSpeed)
		{
			if (!alive)
			{
				ClearSustained(state);
				continue;
			}
			// A zero-length tick (level spawn, pause) carries no movement step: its speed reads as 0, so it must not start or end the state.
			if (deltaSeconds <= 0.f) continue;
			const ModuleSustainedSpeedRule& rule = state.resolved.rule;
			const bool above = cap > 0.f && speed + kSpeedEpsilon >= rule.speedFraction * cap;
			if (above)
			{
				state.fastSeconds += deltaSeconds;
				state.lingering = false;
				state.lingerSeconds = 0.f;
				if (state.fastSeconds + kTimeEpsilon >= rule.holdSeconds) SetSonic(state, true);
				continue;
			}
			state.fastSeconds = 0.f;
			if (state.sonic && !state.lingering)
			{
				state.lingering = true;
				state.lingerSeconds = rule.lingerSeconds;
			}
			if (!state.lingering) continue;
			state.lingerSeconds -= deltaSeconds;
			if (state.lingerSeconds <= kTimeEpsilon)
			{
				state.lingering = false;
				state.lingerSeconds = 0.f;
				SetSonic(state, false);
			}
		}
	}
	void ModuleReactiveRuntime::SpawnSmolderingField(const sf::Vector2f& origin, const ModuleThermalBurstRule& rule)
	{
		if (mShuttingDown || !mShip || !mShip->GetWorld() || mShip->GetIsPendingDestroy() || rule.smolderSeconds <= 0.f || rule.smolderRadius <= 0.f) return;
		const auto* profile = PresentationProfileRegistry<ModuleExplosionPresentationProfile>::Find(ModuleGeneratedPresentationIds::ThermalExplosion.ToString());
		if (!profile) return;
		PruneSmolderFields();
		const auto lease = std::make_shared<ModuleGeneratedLease>();
		lease->key = mNextLeaseKey++;
		const auto actor = mShip->GetWorld()->SpawnActor<ModuleSmolderingFieldActor>(mShip->GetWeakActor(), *profile, lease, rule.smolderSeconds, rule.smolderRadius, rule.smolderStacksPerSecond).lock();
		if (!actor) return;
		actor->SetActorLocation(origin);
		lease->actors.push_back(actor);
		mSmolderLeases.push_back(lease);
	}
	void ModuleReactiveRuntime::PruneSmolderFields()
	{
		mSmolderLeases.erase(std::remove_if(mSmolderLeases.begin(), mSmolderLeases.end(), [](const std::shared_ptr<ModuleGeneratedLease>& lease)
		{
			return std::none_of(lease->actors.begin(), lease->actors.end(), [](const weak_ptr<Actor>& weak) { const auto actor = weak.lock(); return actor && !actor->GetIsPendingDestroy(); });
		}), mSmolderLeases.end());
	}
	void ModuleReactiveRuntime::CancelSmolderFields() noexcept
	{
		for (const auto& lease : mSmolderLeases)
		{
			lease->active = false;
			const auto actors = std::move(lease->actors);
			lease->actors.clear();
			for (const auto& weak : actors) if (const auto actor = weak.lock()) { try { actor->Destroy(); } catch (...) {} }
		}
		mSmolderLeases.clear();
	}
	std::size_t ModuleReactiveRuntime::GetOwnedSmolderFieldCount() const
	{
		std::size_t count = 0;
		for (const auto& lease : mSmolderLeases) for (const auto& weak : lease->actors) if (const auto actor = weak.lock(); actor && !actor->GetIsPendingDestroy()) ++count;
		return count;
	}
	std::size_t ModuleReactiveRuntime::GetOwnedHandleCount() const
	{
		return static_cast<std::size_t>(std::count_if(mAttributes.begin(), mAttributes.end(), [](const AttributeState& state) { return state.handle.IsValid(); }));
	}
	void ModuleReactiveRuntime::NotifyOwnerKill(const sas::AbilityEvent& event)
	{
		const DamageContext* damage = event.GetContext<DamageContext>();
		if (!damage || damage->payload.skipModuleDamageRules || damage->payload.moduleTriggerPolicy == DamageModuleTriggerPolicy::Suppressed ||
			damage->source != &mAbilities.GetOwner() || !damage->targetWasKilled || !damage->targetWasEnemyCombatant) return;
		const GameAbility* ability = mAbilities.GetAbilityById(event.sourceAbilityId.ToString());
		if (!ability || !sas::IsLoadoutAbilitySlot(ability->GetDefinition().slot)) return;
		const auto* target = dynamic_cast<const SpaceShip*>(damage->target);
		mKillObserver(ability->GetDefinition().slot, target ? target->GetControlTargetClass() : ControlTargetClass::Normal);
	}

	void ModuleReactiveRuntime::OnEvent(const sas::AbilityEvent& event)
	{
		if (!mShuttingDown && mShip && mKillObserver && ModuleTriggerFromEventTag(event.eventTag) == ModuleTriggerKind::KillConfirmed) NotifyOwnerKill(event);
		if (!mShuttingDown && mShip && mAbilityUseObserver && event.eventTag == GameplayTags::Event::Ability::Activated && mShip->GetHealthComponent().GetHealth() > 0.f)
		{
			const auto* ability = mAbilities.GetAbilityById(event.sourceAbilityId.ToString());
			if (ability && sas::IsLoadoutAbilitySlot(ability->GetDefinition().slot)) mAbilityUseObserver(ability->GetDefinition().abilityTags);
		}
		if (mShuttingDown || !mShip || mActions.empty()) return;
		const auto* context = event.GetContext<DamageContext>();
		if (context && context->payload.moduleTriggerPolicy == DamageModuleTriggerPolicy::Suppressed) return;
		const ModuleTriggerKind trigger = ModuleTriggerFromEventTag(event.eventTag);
		if (trigger == ModuleTriggerKind::KillConfirmed)
		{
			const DamageContext* damage = event.GetContext<DamageContext>();
			if (!damage || damage->payload.skipModuleDamageRules || damage->source != &mAbilities.GetOwner() || !damage->targetWasKilled || !damage->targetWasEnemyCombatant) return;
		}
		else if (trigger == ModuleTriggerKind::AbilityActivated)
		{
			const GameAbility* ability = mAbilities.GetAbilityById(event.sourceAbilityId.ToString());
			if (!ability || !sas::IsLoadoutAbilitySlot(ability->GetDefinition().slot)) return;
		}
		else if (trigger == ModuleTriggerKind::DamageTaken)
		{
			const auto* damage = event.GetContext<DamageContext>();
			if (!damage || damage->payload.skipModuleDamageRules || damage->target != mShip || damage->appliedDamage <= 0.f) return;
		}
		else return;
		mEvents.push_back({ trigger, event.sourceAbilityId });
		Flush();
	}
	void ModuleReactiveRuntime::OnShield(float amount, float current, float maximum)
	{
		(void)amount; (void)current; (void)maximum; mDirty = true; Flush();
	}
	void ModuleReactiveRuntime::ReplaceConditionalMovement(const List<ResolvedModuleEffects::ConditionalMovement>& rules)
	{
		if (mShuttingDown || !mShip) return;
		for (const auto& rule : mConditionalMovement) mShip->GetRuntimeModifiers().Remove(ConditionalMovementSource(rule.instanceId, rule.ruleIndex));
		mConditionalMovement = rules; UpdateConditionalMovement();
	}
	void ModuleReactiveRuntime::UpdateConditionalMovement()
	{
		if (!mShip || mShuttingDown) return;
		const auto& shield = mShip->GetShieldComponent();
		const bool active = mShip->GetHealthComponent().GetHealth() > 0.f && shield.GetMaxShield() > 0.f && shield.GetShield() > shield.GetMaxShield();
		for (const auto& rule : mConditionalMovement)
		{
			const auto key = ConditionalMovementSource(rule.instanceId, rule.ruleIndex);
			if (active) { ShipRuntimeModifier modifier; modifier.movementSpeedMultiplier = rule.rule.multiplier; mShip->GetRuntimeModifiers().Set(key, modifier); }
			else mShip->GetRuntimeModifiers().Remove(key);
		}
	}
	void ModuleReactiveRuntime::OnHealth(float amount, float health, float maximum)
	{
		(void)amount; (void)health; (void)maximum;
		mDirty = true;
		Flush();
	}
	void ModuleReactiveRuntime::OnAttribute(sas::AttributeId id, float previous, float current)
	{
		(void)previous; (void)current;
		if (id != OwnerAttributeIds::AttackPower && id != OwnerAttributeIds::EnergyPower && id != OwnerAttributeIds::Luck && id != OwnerAttributeIds::Armor) return;
		mDirty = true;
		Flush();
	}
	void ModuleReactiveRuntime::Flush(float deltaSeconds)
	{
		if (mBusy || mShuttingDown) return;
		mBusy = true;
		try
		{
			while (mPending || mDirty || !mEvents.empty())
			{
				if (mPending)
				{
					DesiredRules desired = std::move(*mPending);
					mPending.reset();
					Reconcile(std::move(desired));
					mDirty = true;
				}
				if (mDirty)
				{
					mDirty = false;
					UpdateAttributes(deltaSeconds);
					UpdateConditionalMovement();
					deltaSeconds = 0.f;
					if (mPending) continue;
				}
				if (!mEvents.empty())
				{
					Event event = std::move(mEvents.front());
					mEvents.erase(mEvents.begin());
					Execute(event);
				}
			}
		}
		catch (...) { mBusy = false; throw; }
		mBusy = false;
	}
	void ModuleReactiveRuntime::Reconcile(DesiredRules desired)
	{
		mFades = std::move(desired.fades);
		List<ActionState> actions;
		for (const auto& resolved : desired.actions)
		{
			ActionState next{ resolved, 0 };
			for (const auto& previous : mActions)
				if (previous.resolved.instanceId == resolved.instanceId && previous.resolved.ruleIndex == resolved.ruleIndex && previous.resolved.rule == resolved.rule)
				{
					next.count = previous.count;
					next.cooldownRemaining = previous.cooldownRemaining;
					next.healingRemaining = previous.healingRemaining; next.healingRate = previous.healingRate;
				}
			actions.push_back(std::move(next));
		}
		for (const auto& previous : mActions)
			if (previous.resolved.rule.kind == ModuleEventActionKind::GrantTemporaryShield && std::none_of(actions.begin(), actions.end(), [&previous](const ActionState& next) { return previous.resolved.instanceId == next.resolved.instanceId && previous.resolved.ruleIndex == next.resolved.ruleIndex && previous.resolved.rule == next.resolved.rule; }))
				mShip->GetShieldComponent().RemoveModuleTemporaryShield(TemporaryShieldSource(previous.resolved.instanceId, previous.resolved.ruleIndex));
		mActions = std::move(actions);
		List<AttributeState> attributes;
		for (const auto& resolved : desired.attributes)
		{
			AttributeState next;
			next.resolved = resolved;
			for (auto& previous : mAttributes)
			{
				if (previous.resolved.instanceId != resolved.instanceId || previous.resolved.ruleIndex != resolved.ruleIndex || !(previous.resolved.rule == resolved.rule)) continue;
				next = previous;
				previous.handle = {};
				break;
			}
			attributes.push_back(std::move(next));
		}
		for (auto& previous : mAttributes) Remove(previous);
		mAttributes = std::move(attributes);
	}
	void ModuleReactiveRuntime::Remove(AttributeState& state) noexcept
	{
		const auto handle = state.handle;
		state.handle = {};
		if (!handle.IsValid()) return;
		try { (state.resolved.rule.target == ModuleAttributeTarget::Ship ? mShip->GetShipRuntime().GetAttributes() : mAbilities.GetAttributes()).RemoveModifier(handle); } catch (...) {}
	}
	void ModuleReactiveRuntime::UpdateAttributes(float deltaSeconds)
	{
		if (!mShip || mAttributes.empty()) return;
		auto& attributes = mAbilities.GetAttributes();
		List<sas::AttributeModifierHandle> excluded;
		for (const auto& state : mAttributes) if (state.handle.IsValid() && state.resolved.rule.target == ModuleAttributeTarget::Owner) excluded.push_back(state.handle);
		const float attack = attributes.GetCurrentValueExcludingModifiers(OwnerAttributeIds::AttackPower, excluded);
		const float energy = attributes.GetCurrentValueExcludingModifiers(OwnerAttributeIds::EnergyPower, excluded);
		const float armor = std::max(0.f, attributes.GetCurrentValue(OwnerAttributeIds::Armor));
		const auto& health = mShip->GetHealthComponent();
		const float fraction = health.GetMaxHealth() > 0.f ? health.GetHealth() / health.GetMaxHealth() : 1.f;
		const auto wallet = mGoldWallet.lock();
		const bool alive = health.GetHealth() > 0.f;
		const auto goldArmor = [&wallet, alive](const ModuleOwnerAttributeRule& goldRule) { return wallet && alive ? std::min(goldRule.maximumAmount, std::floor(static_cast<float>(wallet->GetAmount()) * goldRule.amount)) : 0.f; };
		for (auto& state : mAttributes)
		{
			const auto& rule = state.resolved.rule;
			sas::AttributeId id = rule.attributeId;
			float amount = health.GetHealth() > 0.f && fraction <= rule.healthAtMost ? rule.amount : 0.f;
			if (rule.source == ModuleOwnerAttributeSource::HigherPower) id = attack >= energy ? OwnerAttributeIds::AttackPower : OwnerAttributeIds::EnergyPower;
			if (rule.source == ModuleOwnerAttributeSource::ArmorHigherPower) { id = attack >= energy ? OwnerAttributeIds::AttackPower : OwnerAttributeIds::EnergyPower; amount *= armor; }
			if (rule.source == ModuleOwnerAttributeSource::ArmorLowerPower) { id = attack >= energy ? OwnerAttributeIds::EnergyPower : OwnerAttributeIds::AttackPower; amount *= armor; }
			if (rule.source == ModuleOwnerAttributeSource::Armor) amount *= armor;
			if (rule.source == ModuleOwnerAttributeSource::Gold)
			{
				amount = goldArmor(rule);
			}
			if (rule.source == ModuleOwnerAttributeSource::GoldArmorBonus)
			{
				amount = 0.f;
				for (const auto& other : mAttributes) if (other.resolved.instanceId == state.resolved.instanceId && other.resolved.rule.source == ModuleOwnerAttributeSource::Gold) amount += goldArmor(other.resolved.rule) * rule.amount;
			}
			if (rule.source == ModuleOwnerAttributeSource::Luck) amount = health.GetHealth() > 0.f ? std::max(0.f, attributes.GetCurrentValueExcludingModifiers(OwnerAttributeIds::Luck, excluded)) * rule.amount : 0.f;
			if (rule.source == ModuleOwnerAttributeSource::MovementSpeed)
			{
				const float speed = mShip->GetMovementComponent().GetCurrentMovementSpeed();
				const float units = rule.speedStep > 0.f ? std::floor((speed - rule.minimumSpeed) / rule.speedStep) + 1.f : speed / 100.f;
				const float desired = health.GetHealth() > 0.f && speed >= rule.minimumSpeed ? std::max(0.f, units) * rule.amount : 0.f;
				amount = desired;
				float fadeSeconds = rule.fadeSeconds;
				for (const auto& fade : mFades) if (fade.instanceId == state.resolved.instanceId) fadeSeconds = std::max(fadeSeconds, fade.rule.fadeSeconds);
				if (fadeSeconds > 0.f && health.GetHealth() > 0.f)
				{
					if (desired < state.previousDesired) { state.fadeStart = state.appliedAmount; state.fadeElapsed = 0.f; }
					state.fadeElapsed += deltaSeconds;
					amount = std::max(desired, state.fadeStart * std::max(0.f, 1.f - state.fadeElapsed / fadeSeconds));
				}
				else { state.fadeStart = 0.f; state.fadeElapsed = 0.f; }
				state.previousDesired = desired;
			}
			if (state.appliedAttribute == id && state.appliedAmount == amount) continue;
			const auto previous = state.handle;
			auto& targetAttributes = rule.target == ModuleAttributeTarget::Ship ? mShip->GetShipRuntime().GetAttributes() : attributes;
			const auto next = amount > 0.f ? targetAttributes.AddModifier({ id, rule.operation, amount }) : sas::AttributeModifierHandle{};
			state.appliedAttribute = id;
			state.appliedAmount = amount;
			state.handle = next;
			if (previous.IsValid()) targetAttributes.RemoveModifier(previous);
			if (mPending) return;
		}
	}
	void ModuleReactiveRuntime::Execute(const Event& event)
	{
		if (!mShip || mShip->GetHealthComponent().GetHealth() <= 0.f) return;
		for (auto& state : mActions)
		{
			const auto& rule = state.resolved.rule;
			if (rule.trigger != event.trigger || state.cooldownRemaining > 0.f) continue;
			if (!rule.requiredEventTags.empty())
			{
				const auto* ability = mAbilities.GetAbilityById(event.abilityId.ToString());
				if (!ability || !std::all_of(rule.requiredEventTags.begin(), rule.requiredEventTags.end(), [ability](const GameplayTag& required)
				{
					const auto& tags = ability->GetDefinition().abilityTags;
					return std::any_of(tags.begin(), tags.end(), [&required](const GameplayTag& tag) { return tag.MatchesTag(required); });
				})) continue;
			}
			if (++state.count < rule.everyCount) continue;
			state.count = 0;
			if (rule.kind == ModuleEventActionKind::ApplyNearbyCryo)
			{
				if (!mShip->GetWorld()) continue;
				const auto ownerLifetime = mShip->GetWeakActor().lock();
				if (!ownerLifetime || mShip->GetIsPendingDestroy()) continue;
				auto targets = targeting::FindOpposingCombatants(*mShip->GetWorld(), *mShip, rule.radius, false);
				const auto origin = mShip->GetActorLocation();
				std::sort(targets.begin(), targets.end(), [origin](const auto& a, const auto& b)
				{
					const auto da = a->GetActorLocation() - origin;
					const auto db = b->GetActorLocation() - origin;
					const float left = da.x * da.x + da.y * da.y;
					const float right = db.x * db.x + db.y * db.y;
					return left == right ? a->GetUniqueID() < b->GetUniqueID() : left < right;
				});
				state.cooldownRemaining = rule.cooldownSeconds;
				for (std::size_t index = 0; index < targets.size() && index < static_cast<std::size_t>(rule.targetCount); ++index)
				{
					if (targets[index]->GetIsPendingDestroy()) continue;
					auto* target = dynamic_cast<Combatant*>(targets[index].get());
					if (!target) continue;
					DamageContext context;
					context.source = mShip;
					context.target = targets[index].get();
					context.remainingDamage = 1.f; // Status application only; no damage resolution or extra hit.
					context.damageTags = { DamageTypeSchema::Cryo };
					context.payload.kineticStacks = 0;
					context.payload.cryoBuildupPerHit = static_cast<int>(rule.amount);
					context.payload.skipModuleDamageRules = true;
					DamageTypeSystem::ApplyStatusEffects(target->GetAbilitySystemComponent(), context);
					if (mPending || mShip->GetIsPendingDestroy()) break;
				}
			}
			else if (rule.kind == ModuleEventActionKind::RefundUsedCooldown)
			{
				const float chance = std::clamp(mAbilities.GetAttributes().GetCurrentValue(OwnerAttributeIds::Luck) * rule.chancePerLuck, 0.f, 1.f);
				if (chance <= 0.f || (chance < 1.f && std::generate_canonical<float, 24>(mRandom) >= chance)) continue;
				if (auto* ability = mAbilities.GetAbilityById(event.abilityId.ToString())) ability->RefundActivationCooldown(rule.amount);
			}
			else if (rule.kind == ModuleEventActionKind::GrantTemporaryShield)
			{
				auto& shield = mShip->GetShieldComponent();
				shield.GrantModuleTemporaryShield(TemporaryShieldSource(state.resolved.instanceId, state.resolved.ruleIndex), shield.GetMaxShield() * rule.amount, rule.durationSeconds);
			}
			else if (rule.kind == ModuleEventActionKind::RestoreShield)
			{
				auto& shield = mShip->GetShieldComponent();
				const float grant = shield.GetMaxShield() * rule.amount;
				const float overflow = std::max(0.f, grant - std::max(0.f, shield.GetMaxShield() - shield.GetShield()));
				shield.ChangeShield(grant);
				float conversion = rule.overflowHealthFraction;
				for (const auto& companion : mActions)
					if (companion.resolved.instanceId == state.resolved.instanceId && companion.resolved.rule.kind == ModuleEventActionKind::ShieldOverflowToHealth)
						conversion += companion.resolved.rule.amount;
				mShip->GetHealthComponent().Regenerate(overflow * conversion);
			}
			else if (rule.kind == ModuleEventActionKind::ShieldOverflowToHealth) continue;
			else if (rule.kind == ModuleEventActionKind::AwardGold) { if (const auto wallet = mGoldWallet.lock()) wallet->Award(static_cast<unsigned int>(rule.amount)); }
			else if (rule.kind == ModuleEventActionKind::RestoreHealthOverTime)
			{
				state.healingRemaining = rule.durationSeconds; state.healingRate = mShip->GetHealthComponent().GetMaxHealth() * rule.amount / rule.durationSeconds;
				state.cooldownRemaining = rule.cooldownSeconds;
			}
			else if (rule.kind == ModuleEventActionKind::RestoreHealth) mShip->GetHealthComponent().Regenerate(mShip->GetHealthComponent().GetMaxHealth() * rule.amount);
			else if (rule.kind == ModuleEventActionKind::RestoreAfterburner) mShip->GetEnergyComponent().Restore(mShip->GetEnergyComponent().GetMaxEnergy() * rule.amount);
			else
			{
				GameAbility* longest = nullptr;
				for (auto slot : { sas::AbilitySlot::Ability1, sas::AbilitySlot::Ability2, sas::AbilitySlot::Ability3, sas::AbilitySlot::Ability4 })
				{
					GameAbility* ability = mAbilities.GetAbility(slot);
					if (!ability || ability->GetCooldownRemaining() <= 0.f) continue;
					if (rule.abilityFamily.IsValid() && !std::any_of(ability->GetDefinition().abilityTags.begin(), ability->GetDefinition().abilityTags.end(), [&rule](const GameplayTag& tag) { return tag.MatchesTag(rule.abilityFamily); })) continue;
					if (rule.kind == ModuleEventActionKind::ReduceAllCooldowns) ability->ReduceCooldownRemaining(rule.amount);
					else if (rule.kind == ModuleEventActionKind::ReduceFamilyCooldowns) ability->ReduceCooldownRemaining(ability->GetCooldownRemaining() * rule.amount);
					else if (!longest || ability->GetCooldownRemaining() > longest->GetCooldownRemaining()) longest = ability;
					if (mPending) break;
				}
				if (longest && !mPending) longest->ReduceCooldownRemaining(longest->GetCooldownRemaining() * rule.amount);
			}
			if (mPending) return;
		}
	}
}
