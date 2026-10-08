#include "gameplay/module/ModuleReactiveRuntime.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/attributes/AttributeIds.h"
#include "spaceShip/SpaceShip.h"
#include "gameplay/targeting/CombatantTargetQuery.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/combat/Combatant.h"
#include <algorithm>

namespace ly
{
	namespace
	{
		bool SameAction(const ModuleEventAction& a, const ModuleEventAction& b)
		{
			return a.trigger == b.trigger && a.kind == b.kind && a.amount == b.amount && a.chancePerLuck == b.chancePerLuck && a.everyCount == b.everyCount && a.overflowHealthFraction == b.overflowHealthFraction && a.radius == b.radius && a.cooldownSeconds == b.cooldownSeconds && a.targetCount == b.targetCount;
		}
		bool SameAttribute(const ModuleOwnerAttributeRule& a, const ModuleOwnerAttributeRule& b)
		{
			return a.source == b.source && a.attributeId == b.attributeId && a.healthAtMost == b.healthAtMost && a.amount == b.amount && a.fadeSeconds == b.fadeSeconds && a.target == b.target && a.operation == b.operation;
		}
	}
	ModuleReactiveRuntime::ModuleReactiveRuntime(LightYearsAbilitySystemComponent& abilities) : mAbilities{ abilities }, mShip{ dynamic_cast<SpaceShip*>(&abilities.GetOwner()) }
	{
		try
		{
			mEventHandle = mAbilities.onGameplayEvent.BindAction(this, &ModuleReactiveRuntime::OnEvent);
			mAttributeHandle = mAbilities.GetAttributes().onAttributeChanged.BindAction(this, &ModuleReactiveRuntime::OnAttribute);
			if (mShip) mHealthHandle = mShip->GetHealthComponent().onHealthChanged.BindAction(this, &ModuleReactiveRuntime::OnHealth);
		}
		catch (...)
		{
			mAbilities.onGameplayEvent.UnbindAction(mEventHandle);
			mAbilities.GetAttributes().onAttributeChanged.UnbindAction(mAttributeHandle);
			if (mShip) mShip->GetHealthComponent().onHealthChanged.UnbindAction(mHealthHandle);
			throw;
		}
	}
	ModuleReactiveRuntime::~ModuleReactiveRuntime()
	{
		mShuttingDown = true;
		mAbilities.onGameplayEvent.UnbindAction(mEventHandle);
		mAbilities.GetAttributes().onAttributeChanged.UnbindAction(mAttributeHandle);
		if (mShip) mShip->GetHealthComponent().onHealthChanged.UnbindAction(mHealthHandle);
		for (AttributeState& state : mAttributes) Remove(state);
	}
	void ModuleReactiveRuntime::Replace(const List<ResolvedModuleEventAction>& actions, const List<ResolvedModuleOwnerAttributeRule>& attributes, const List<ResolvedModuleEffects::MovementFade>& fades)
	{
		if (mShuttingDown) return;
		mPending = DesiredRules{ actions, attributes, fades };
		Flush();
	}
	void ModuleReactiveRuntime::Tick(float deltaSeconds)
	{
		if (deltaSeconds < 0.f || mShuttingDown) return;
		for (auto& state : mActions) state.cooldownRemaining = std::max(0.f, state.cooldownRemaining - deltaSeconds);
		mDirty = true;
		Flush(deltaSeconds);
	}
	std::size_t ModuleReactiveRuntime::GetOwnedHandleCount() const
	{
		return static_cast<std::size_t>(std::count_if(mAttributes.begin(), mAttributes.end(), [](const AttributeState& state) { return state.handle.IsValid(); }));
	}
	void ModuleReactiveRuntime::OnEvent(const sas::AbilityEvent& event)
	{
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
	void ModuleReactiveRuntime::OnHealth(float amount, float health, float maximum)
	{
		(void)amount; (void)health; (void)maximum;
		mDirty = true;
		Flush();
	}
	void ModuleReactiveRuntime::OnAttribute(sas::AttributeId id, float previous, float current)
	{
		(void)previous; (void)current;
		if (id != OwnerAttributeIds::AttackPower && id != OwnerAttributeIds::EnergyPower && id != OwnerAttributeIds::Luck) return;
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
				if (previous.resolved.instanceId == resolved.instanceId && previous.resolved.ruleIndex == resolved.ruleIndex && SameAction(previous.resolved.rule, resolved.rule))
				{
					next.count = previous.count;
					next.cooldownRemaining = previous.cooldownRemaining;
				}
			actions.push_back(std::move(next));
		}
		mActions = std::move(actions);
		List<AttributeState> attributes;
		for (const auto& resolved : desired.attributes)
		{
			AttributeState next;
			next.resolved = resolved;
			for (auto& previous : mAttributes)
			{
				if (previous.resolved.instanceId != resolved.instanceId || previous.resolved.ruleIndex != resolved.ruleIndex || !SameAttribute(previous.resolved.rule, resolved.rule)) continue;
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
		const auto& health = mShip->GetHealthComponent();
		const float fraction = health.GetMaxHealth() > 0.f ? health.GetHealth() / health.GetMaxHealth() : 1.f;
		for (auto& state : mAttributes)
		{
			const auto& rule = state.resolved.rule;
			sas::AttributeId id = rule.attributeId;
			float amount = health.GetHealth() > 0.f && fraction <= rule.healthAtMost ? rule.amount : 0.f;
			if (rule.source == ModuleOwnerAttributeSource::HigherPower) id = attack >= energy ? OwnerAttributeIds::AttackPower : OwnerAttributeIds::EnergyPower;
			if (rule.source == ModuleOwnerAttributeSource::Luck) amount = health.GetHealth() > 0.f ? std::max(0.f, attributes.GetCurrentValueExcludingModifiers(OwnerAttributeIds::Luck, excluded)) * rule.amount : 0.f;
			if (rule.source == ModuleOwnerAttributeSource::MovementSpeed)
			{
				const float desired = health.GetHealth() > 0.f ? mShip->GetMovementComponent().GetCurrentMovementSpeed() * rule.amount / 100.f : 0.f;
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
			if (rule.trigger != event.trigger || state.cooldownRemaining > 0.f || ++state.count < rule.everyCount) continue;
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
			else if (rule.kind == ModuleEventActionKind::RestoreHealth) mShip->GetHealthComponent().Regenerate(mShip->GetHealthComponent().GetMaxHealth() * rule.amount);
			else if (rule.kind == ModuleEventActionKind::RestoreAfterburner) mShip->GetEnergyComponent().Restore(mShip->GetEnergyComponent().GetMaxEnergy() * rule.amount);
			else
			{
				GameAbility* longest = nullptr;
				for (auto slot : { sas::AbilitySlot::Ability1, sas::AbilitySlot::Ability2, sas::AbilitySlot::Ability3, sas::AbilitySlot::Ability4 })
				{
					GameAbility* ability = mAbilities.GetAbility(slot);
					if (!ability || ability->GetCooldownRemaining() <= 0.f) continue;
					if (rule.kind == ModuleEventActionKind::ReduceAllCooldowns) ability->ReduceCooldownRemaining(rule.amount);
					else if (!longest || ability->GetCooldownRemaining() > longest->GetCooldownRemaining()) longest = ability;
					if (mPending) break;
				}
				if (longest && !mPending) longest->ReduceCooldownRemaining(longest->GetCooldownRemaining() * rule.amount);
			}
			if (mPending) return;
		}
	}
}
