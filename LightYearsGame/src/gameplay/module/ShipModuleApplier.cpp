#include "gameplay/module/ShipModuleApplier.h"

#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/module/ModuleTriggerRuntime.h"
#include "gameplay/tags/GameplayTags.h"

#include <algorithm>
#include <utility>

namespace ly
{
	namespace
	{
		bool SameModifier(const sas::AttributeModifier& a, const sas::AttributeModifier& b)
		{
			return a.attributeId == b.attributeId && a.operation == b.operation &&
				a.magnitude == b.magnitude && a.priority == b.priority;
		}
	}

	ShipModuleApplier::ShipModuleApplier(
		ShipRuntime& shipRuntime,
		LightYearsAbilitySystemComponent& abilities,
		ModuleTriggerRuntime* triggerRuntime,
		CombatRuntime* combatRuntime
	)
		: mShipRuntime{ shipRuntime }
		, mAbilities{ abilities }
		, mTriggerRuntime{ triggerRuntime }
		, mDamageRuleRegistration{ combatRuntime ? combatRuntime->RegisterOutgoingDamageRules() : CombatRuntime::OutgoingDamageRuleRegistration{} }
	{
		// The applier and the ability system component belong to the same ship and the
		// applier is destroyed first, so clearing the gate here leaves no dangling pointer.
		mAbilities.SetModuleSystemGate(this);
	}

	ShipModuleApplier::~ShipModuleApplier()
	{
		mAbilities.SetModuleSystemGate(nullptr);
	}

	bool ShipModuleApplier::IsSystemEnabled(const GameplayTag& systemTag) const
	{
		return std::find(mEnabledSystems.begin(), mEnabledSystems.end(), systemTag) != mEnabledSystems.end();
	}

	bool ShipModuleApplier::IsRebooting(std::uint32_t instanceId) const
	{
		return GetRebootRemainingSeconds(instanceId) > 0.f;
	}

	float ShipModuleApplier::GetRebootRemainingSeconds(std::uint32_t instanceId) const
	{
		for (const RebootEntry& entry : mReboots)
		{
			if (entry.instanceId == instanceId) return entry.remainingSeconds;
		}
		return 0.f;
	}

	bool ShipModuleApplier::Apply(const ModuleLoadout& loadout, const ModuleRulesContext& rules, bool inCombat)
	{
		if (mHasApplied && mAppliedRevision == loadout.GetRevision()) return false;

		const float rebootDuration = rules.zoneTable.rebootDurationSeconds;
		List<RebootEntry> reboots;
		List<std::uint32_t> knownIds;
		for (const ModulePlacement& placement : loadout.GetPlacements())
		{
			knownIds.push_back(placement.instanceId);
			const auto running = std::find_if(mReboots.begin(), mReboots.end(),
				[&placement](const RebootEntry& entry) { return entry.instanceId == placement.instanceId; });
			if (running != mReboots.end())
			{
				reboots.push_back(*running);
				continue;
			}
			const bool isNew = std::find(mKnownIds.begin(), mKnownIds.end(), placement.instanceId) == mKnownIds.end();
			if (mHasApplied && isNew && inCombat && rebootDuration > 0.f)
			{
				reboots.push_back({ placement.instanceId, rebootDuration });
			}
		}

		mReboots = std::move(reboots);
		mKnownIds = std::move(knownIds);
		mPlacements = loadout.GetPlacements();
		mSchematic = rules.schematic;
		mZoneTable = rules.zoneTable;
		mLookup = rules.lookup;
		mAppliedRevision = loadout.GetRevision();
		Rebuild();
		mHasApplied = true;
		return true;
	}

	void ShipModuleApplier::Tick(float deltaSeconds)
	{
		if (!mHasApplied || mReboots.empty() || deltaSeconds <= 0.f) return;

		bool anyFinished = false;
		for (RebootEntry& entry : mReboots) entry.remainingSeconds -= deltaSeconds;
		const auto finished = std::remove_if(mReboots.begin(), mReboots.end(),
			[&anyFinished](const RebootEntry& entry)
			{
				if (entry.remainingSeconds > 0.f) return false;
				anyFinished = true;
				return true;
			});
		mReboots.erase(finished, mReboots.end());
		if (anyFinished) Rebuild();
	}

	void ShipModuleApplier::Rebuild()
	{
		List<ModulePlacement> active;
		active.reserve(mPlacements.size());
		for (const ModulePlacement& placement : mPlacements)
		{
			if (!IsRebooting(placement.instanceId)) active.push_back(placement);
		}

		const ModuleRulesContext rules{ mSchematic, mLookup, mZoneTable };
		mResolved = ModuleResolver::Resolve(rules, active);
		mDamageRuleRegistration.ReplaceRules(mResolved.damageRules);

		// Modifiers that stay identical keep their handles and new ones are added before
		// stale ones are removed, so a reapply never dips a max value below both the old
		// and the new result; the current shield and afterburner energy are not clipped
		// by an unrelated change.
		SyncModifiers(mAbilities.GetAttributes(), mResolved.ownerModifiers, mOwnerHandles);
		SyncModifiers(mShipRuntime.GetAttributes(), mResolved.shipModifiers, mShipHandles);
		SyncPrimaryWeaponModifiers();
		RemoveScopedRules();
		for (const ScopedAbilityRule& rule : mResolved.abilityRules)
		{
			mScopedRuleHandles.push_back(mAbilities.AddScopedAbilityRule(rule));
		}
		if (mTriggerRuntime) mTriggerRuntime->SetRules(mResolved.triggerBuffs);

		mEnabledSystems = mResolved.enabledSystems;
		mShipRuntime.SetSystemEnabled(ShipSystem::Shield, IsSystemEnabled(GameplayTags::Module::System::Shield));
		mShipRuntime.SetSystemEnabled(ShipSystem::Afterburner, IsSystemEnabled(GameplayTags::Module::System::Afterburner));
		mAbilities.CancelAbilitiesUnavailableByModules();
	}

	void ShipModuleApplier::SyncModifiers(
		sas::AttributeSystem& attributes,
		const List<sas::AttributeModifier>& desired,
		List<OwnedModifier>& owned)
	{
		List<OwnedModifier> next;
		next.reserve(desired.size());
		List<bool> reused(owned.size(), false);
		for (const sas::AttributeModifier& modifier : desired)
		{
			bool matched = false;
			for (std::size_t i = 0; i < owned.size(); ++i)
			{
				if (reused[i] || !SameModifier(owned[i].modifier, modifier)) continue;
				reused[i] = true;
				next.push_back(owned[i]);
				matched = true;
				break;
			}
			if (matched) continue;
			const sas::AttributeModifierHandle handle = attributes.AddModifier(modifier);
			if (handle.IsValid()) next.push_back({ modifier, handle });
		}
		for (std::size_t i = 0; i < owned.size(); ++i)
		{
			if (!reused[i]) attributes.RemoveModifier(owned[i].handle);
		}
		owned = std::move(next);
	}

	void ShipModuleApplier::SyncPrimaryWeaponModifiers()
	{
		const List<sas::AttributeModifier>& desired = mResolved.primaryWeaponModifiers;
		List<OwnedPrimaryModifier> next;
		List<bool> reused(mPrimaryHandles.size(), false);
		for (const sas::AttributeModifier& modifier : desired)
		{
			bool matched = false;
			for (std::size_t i = 0; i < mPrimaryHandles.size() && !matched; ++i)
			{
				if (reused[i] || !SameModifier(mPrimaryHandles[i].modifier, modifier)) continue;
				reused[i] = true;
				next.push_back(mPrimaryHandles[i]);
				matched = true;
			}
			if (!matched) next.push_back({ modifier, mAbilities.AddPrimaryWeaponModifier(modifier) });
		}
		for (std::size_t i = 0; i < mPrimaryHandles.size(); ++i)
		{
			if (!reused[i]) mAbilities.RemovePrimaryWeaponModifier(mPrimaryHandles[i].handle);
		}
		mPrimaryHandles = std::move(next);
	}

	void ShipModuleApplier::RemoveScopedRules()
	{
		for (const std::size_t handle : mScopedRuleHandles) mAbilities.RemoveScopedAbilityRule(handle);
		mScopedRuleHandles.clear();
	}
}
