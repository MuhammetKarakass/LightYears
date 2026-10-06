#include "gameplay/module/ModuleTriggerRuntime.h"

#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/damage/DamageContext.h"
#include "gameplay/ShieldComponent.h"
#include "gameplay/tags/GameplayTags.h"

#include <algorithm>
#include <string>
#include <utility>

namespace ly
{
	namespace
	{
		class DispatchDepthScope final
		{
		public:
			explicit DispatchDepthScope(std::size_t& depth) : mDepth{ depth } { ++mDepth; }
			~DispatchDepthScope() { --mDepth; }
			DispatchDepthScope(const DispatchDepthScope&) = delete;
			DispatchDepthScope& operator=(const DispatchDepthScope&) = delete;

		private:
			std::size_t& mDepth;
		};
	}

	ModuleTriggerRuntime::ModuleTriggerRuntime(
		LightYearsAbilitySystemComponent& abilities,
		ShipRuntime& shipRuntime,
		ShipRuntimeModifiers& runtimeModifiers,
		CombatRuntime& combatRuntime,
		ShieldComponent& shield
	)
		: mAbilities{ abilities },
		  mShipRuntime{ shipRuntime },
		  mRuntimeModifiers{ runtimeModifiers },
		  mCombatRuntime{ combatRuntime },
		  mShield{ shield }
	{
		mGameplayEventHandle = mAbilities.onGameplayEvent.BindAction(this, &ModuleTriggerRuntime::OnGameplayEvent);
		mShieldDamagedHandle = mShield.onShieldDamaged.BindAction(this, &ModuleTriggerRuntime::OnShieldDamaged);
	}

	ModuleTriggerRuntime::~ModuleTriggerRuntime()
	{
		mShuttingDown = true;
		mAbilities.onGameplayEvent.UnbindAction(mGameplayEventHandle);
		mShield.onShieldDamaged.UnbindAction(mShieldDamagedHandle);
		mPendingRules.reset();
		mPendingEvents.clear();
		DispatchDepthScope dispatchScope{ mDispatchDepth };
		while (!mActiveBuffs.empty()) RemoveActiveBuff(mActiveBuffs.size() - 1);
		mRules.clear();
	}

	void ModuleTriggerRuntime::SetRules(const List<ResolvedModuleTriggerBuff>& rules, std::uint64_t& generationOut)
	{
		if (mShuttingDown)
		{
			generationOut = 0;
			return;
		}
		List<ResolvedModuleTriggerBuff> desired = rules;
		std::uint64_t generation = mNextRulesGeneration++;
		if (generation == 0) generation = mNextRulesGeneration++;
		mPendingRules = std::move(desired);
		mRulesGeneration = generation;
		generationOut = generation;
		ProcessDeferredUpdates();
	}

	void ModuleTriggerRuntime::ClearRules(std::uint64_t generation)
	{
		if (mShuttingDown || generation == 0 || generation != mRulesGeneration) return;
		std::uint64_t clearedGeneration = 0;
		SetRules({}, clearedGeneration);
	}

	void ModuleTriggerRuntime::Tick(float deltaSeconds)
	{
		if (mShuttingDown || deltaSeconds <= 0.f || mDispatchDepth != 0) return;
		{
			DispatchDepthScope dispatchScope{ mDispatchDepth };
			for (std::size_t index = mActiveBuffs.size(); index > 0; --index)
			{
				ActiveBuff& buff = mActiveBuffs[index - 1];
				buff.remainingSeconds -= deltaSeconds;
				if (buff.remainingSeconds <= 0.f) RemoveActiveBuff(index - 1);
			}
		}
		ProcessDeferredUpdates();
	}

	std::size_t ModuleTriggerRuntime::GetActiveModifierHandleCount() const
	{
		std::size_t count = 0;
		for (const ActiveBuff& buff : mActiveBuffs)
		{
			count += buff.modifiers.size();
			if (buff.ownsMovementModifier) ++count;
		}
		return count;
	}

	void ModuleTriggerRuntime::OnGameplayEvent(const sas::AbilityEvent& event)
	{
		if (mShuttingDown) return;
		const DamageContext* damage = event.GetContext<DamageContext>();
		mPendingEvents.push_back({ ModuleTriggerFromEventTag(event.eventTag), event.magnitude, event.sourceAbilityTags,
			event.payloadTags, damage && damage->payload.skipModuleDamageRules });
		ProcessDeferredUpdates();
	}

	void ModuleTriggerRuntime::OnShieldDamaged(float absorbed, float remainingShield, float maxShield)
	{
		(void)maxShield;
		if (!mShuttingDown && absorbed > 0.f && remainingShield == 0.f)
		{
			mPendingEvents.push_back({ ModuleTriggerKind::ShieldBroken, absorbed, {}, {} });
			ProcessDeferredUpdates();
		}
	}

	void ModuleTriggerRuntime::ProcessEvent(const EventSnapshot& event)
	{
		if (!IsModuleTriggerEventEligible(event.trigger, event.magnitude)) return;
		mCombatRuntime.ActivateOutgoingDamageRules(
			{ event.trigger, event.magnitude, event.sourceAbilityTags, event.payloadTags, event.moduleGenerated });
		List<ResolvedModuleTriggerBuff> matchingRules;
		for (const ResolvedModuleTriggerBuff& resolved : mRules)
		{
			if (MatchesTrigger(resolved.rule, event)) matchingRules.push_back(resolved);
		}

		for (const ResolvedModuleTriggerBuff& resolved : matchingRules)
		{
			if (mPendingRules) break;
			const auto active = std::find_if(
				mActiveBuffs.begin(),
				mActiveBuffs.end(),
				[&resolved](const ActiveBuff& buff) { return SameKey(buff, resolved); }
			);
			if (active != mActiveBuffs.end())
			{
				active->remainingSeconds = resolved.rule.durationSeconds;
				continue;
			}
			ActiveBuff buff = CreateActiveBuff(resolved);
			try { mActiveBuffs.push_back(std::move(buff)); }
			catch (...)
			{
				RemoveBuffEffects(buff);
				throw;
			}
		}
	}

	void ModuleTriggerRuntime::ProcessDeferredUpdates()
	{
		if (mShuttingDown || mDispatchDepth != 0) return;
		while (mPendingRules || !mPendingEvents.empty())
		{
			if (mPendingRules)
			{
				List<ResolvedModuleTriggerBuff> rules = std::move(*mPendingRules);
				mPendingRules.reset();
				DispatchDepthScope dispatchScope{ mDispatchDepth };
				ReconcileRules(rules);
				continue;
			}

			EventSnapshot event = std::move(mPendingEvents.front());
			mPendingEvents.erase(mPendingEvents.begin());
			DispatchDepthScope dispatchScope{ mDispatchDepth };
			ProcessEvent(event);
		}
	}

	void ModuleTriggerRuntime::ReconcileRules(const List<ResolvedModuleTriggerBuff>& rules)
	{
		List<ResolvedModuleTriggerBuff> nextRules = rules;
		for (std::size_t index = mActiveBuffs.size(); index > 0; --index)
		{
			const ActiveBuff& active = mActiveBuffs[index - 1];
			const auto desired = std::find_if(
				rules.begin(),
				rules.end(),
				[&active](const ResolvedModuleTriggerBuff& resolved) { return SameKey(active, resolved); }
			);
			if (desired == rules.end() || !SameRule(active.rule, desired->rule))
			{
				RemoveActiveBuff(index - 1);
			}
		}
		mRules = std::move(nextRules);
	}

	void ModuleTriggerRuntime::RemoveActiveBuff(std::size_t index)
	{
		if (index >= mActiveBuffs.size()) return;
		ActiveBuff buff = std::move(mActiveBuffs[index]);
		mActiveBuffs.erase(mActiveBuffs.begin() + static_cast<std::ptrdiff_t>(index));
		RemoveBuffEffects(buff);
	}

	void ModuleTriggerRuntime::RemoveBuffEffects(ActiveBuff& buff)
	{
		for (const OwnedAttributeModifier& modifier : buff.modifiers)
		{
			try
			{
				switch (modifier.target)
				{
				case ModuleAttributeTarget::Owner:
					mAbilities.GetAttributes().RemoveModifier(modifier.attributeHandle);
					break;
				case ModuleAttributeTarget::Ship:
					mShipRuntime.GetAttributes().RemoveModifier(modifier.attributeHandle);
					break;
				case ModuleAttributeTarget::PrimaryWeapon:
					mAbilities.RemovePrimaryWeaponModifier(modifier.primaryWeaponHandle);
					break;
				}
			}
			catch (...) {}
		}
		if (buff.ownsMovementModifier)
		{
			try { mRuntimeModifiers.Remove(buff.movementSourceId); }
			catch (...) {}
		}
	}

	ModuleTriggerRuntime::ActiveBuff ModuleTriggerRuntime::CreateActiveBuff(
		const ResolvedModuleTriggerBuff& resolved
	)
	{
		ActiveBuff buff;
		buff.instanceId = resolved.instanceId;
		buff.ruleIndex = resolved.ruleIndex;
		buff.rule = resolved.rule;
		buff.remainingSeconds = resolved.rule.durationSeconds;
		buff.movementSourceId = MakeMovementSourceId(resolved.instanceId, resolved.ruleIndex);
		buff.modifiers.reserve(resolved.rule.modifiers.size());
		try
		{
			for (const ModuleAttributeModifier& modifier : resolved.rule.modifiers)
			{
				OwnedAttributeModifier owned;
				owned.target = modifier.target;
				switch (modifier.target)
				{
				case ModuleAttributeTarget::Owner:
					owned.attributeHandle = mAbilities.GetAttributes().AddModifier(modifier.modifier);
					break;
				case ModuleAttributeTarget::Ship:
					owned.attributeHandle = mShipRuntime.GetAttributes().AddModifier(modifier.modifier);
					break;
				case ModuleAttributeTarget::PrimaryWeapon:
					owned.primaryWeaponHandle = mAbilities.AddPrimaryWeaponModifier(modifier.modifier);
					break;
				}
				if ((modifier.target == ModuleAttributeTarget::PrimaryWeapon && owned.primaryWeaponHandle != 0) ||
					(modifier.target != ModuleAttributeTarget::PrimaryWeapon && owned.attributeHandle.IsValid()))
				{
					buff.modifiers.push_back(owned);
				}
			}
			if (resolved.rule.movementSpeedMultiplier != 1.f)
			{
				ShipRuntimeModifier modifier;
				modifier.movementSpeedMultiplier = resolved.rule.movementSpeedMultiplier;
				mRuntimeModifiers.Set(buff.movementSourceId, modifier);
				buff.ownsMovementModifier = true;
			}
		}
		catch (...)
		{
			RemoveBuffEffects(buff);
			throw;
		}
		return buff;
	}

	bool ModuleTriggerRuntime::MatchesTrigger(
		const ModuleTriggerBuff& rule,
		const EventSnapshot& event
	) const
	{
		return MatchesModuleTrigger(rule.trigger, rule.requiredEventTags, rule.durationSeconds,
			{ event.trigger, event.magnitude, event.sourceAbilityTags, event.payloadTags, event.moduleGenerated });
	}

	bool ModuleTriggerRuntime::SameKey(
		const ActiveBuff& active,
		const ResolvedModuleTriggerBuff& resolved
	)
	{
		return active.instanceId == resolved.instanceId && active.ruleIndex == resolved.ruleIndex;
	}

	bool ModuleTriggerRuntime::SameRule(const ModuleTriggerBuff& left, const ModuleTriggerBuff& right)
	{
		if (left.trigger != right.trigger || left.durationSeconds != right.durationSeconds ||
			left.movementSpeedMultiplier != right.movementSpeedMultiplier ||
			left.requiredEventTags.size() != right.requiredEventTags.size() ||
			left.modifiers.size() != right.modifiers.size())
		{
			return false;
		}

		for (std::size_t index = 0; index < left.requiredEventTags.size(); ++index)
		{
			if (left.requiredEventTags[index] != right.requiredEventTags[index]) return false;
		}
		for (std::size_t index = 0; index < left.modifiers.size(); ++index)
		{
			const ModuleAttributeModifier& leftModifier = left.modifiers[index];
			const ModuleAttributeModifier& rightModifier = right.modifiers[index];
			if (leftModifier.target != rightModifier.target ||
				leftModifier.affectedByZoneBonus != rightModifier.affectedByZoneBonus ||
				leftModifier.modifier.attributeId != rightModifier.modifier.attributeId ||
				leftModifier.modifier.operation != rightModifier.modifier.operation ||
				leftModifier.modifier.magnitude != rightModifier.modifier.magnitude ||
				leftModifier.modifier.priority != rightModifier.modifier.priority)
			{
				return false;
			}
		}
		return true;
	}

	std::string ModuleTriggerRuntime::MakeMovementSourceId(std::uint32_t instanceId, std::size_t ruleIndex)
	{
		return "ModuleTrigger." + std::to_string(instanceId) + "." + std::to_string(ruleIndex);
	}
}
