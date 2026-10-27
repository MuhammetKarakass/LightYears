#include "gameplay/module/ShipModuleApplier.h"

#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/module/ModuleAbilitySlots.h"
#include "gameplay/module/ModuleTriggerRuntime.h"
#include "gameplay/tags/GameplayTags.h"
#include "spaceShip/SpaceShip.h"

#include <algorithm>
#include <utility>

namespace vega
{
	namespace
	{
		struct ModuleMovementBehavior
		{
			float thrustBonus = 0.f;
			bool equalizeDirections = false;
		};

		// Ship-wide thrust shaping from every placed movementRules entry. The smallest thrustMultiplier wins (the
		// entries do not stack) and equalizeDirections holds when any entry asks for it.
		ModuleMovementBehavior ResolveMovementBehavior(const List<ResolvedModuleEffects::MovementRule>& rules)
		{
			float thrustMultiplier = 1.f;
			ModuleMovementBehavior behavior;
			for (const ResolvedModuleEffects::MovementRule& resolved : rules)
			{
				thrustMultiplier = std::min(thrustMultiplier, resolved.rule.thrustMultiplier);
				behavior.equalizeDirections = behavior.equalizeDirections || resolved.rule.equalizeDirections;
			}
			behavior.thrustBonus = thrustMultiplier - 1.f;
			return behavior;
		}

		// Ship-wide shield behaviour from every placed shieldRules entry. The regen mode with the highest
		// priority wins (Continuous over SpeedScaled over Normal; ties keep placement order) and supplies its own
		// rate values. The re-arm and armor-share fractions take the largest value across all entries.
		ShieldModuleBehavior ResolveShieldBehavior(const List<ResolvedModuleEffects::ShieldRule>& rules)
		{
			const auto priority = [](ModuleShieldRegenMode mode) { return mode == ModuleShieldRegenMode::Continuous ? 2 : mode == ModuleShieldRegenMode::SpeedScaled ? 1 : 0; };
			ShieldModuleBehavior behavior;
			const ModuleShieldRule* winner = nullptr;
			for (const ResolvedModuleEffects::ShieldRule& resolved : rules)
			{
				const ModuleShieldRule& rule = resolved.rule;
				behavior.breakRearmFraction = std::max(behavior.breakRearmFraction, rule.breakRearmFraction);
				behavior.armorShare = std::max(behavior.armorShare, rule.armorShare);
				behavior.frontRearSplit = behavior.frontRearSplit || rule.frontRearSplit;
				if (rule.regenMode == ModuleShieldRegenMode::Normal) continue;
				if (!winner || priority(rule.regenMode) > priority(winner->regenMode)) winner = &rule;
			}
			if (winner)
			{
				behavior.regenMode = winner->regenMode == ModuleShieldRegenMode::Continuous
					? ShieldModuleBehavior::RegenMode::Continuous
					: ShieldModuleBehavior::RegenMode::SpeedScaled;
				behavior.speedMultiplierAtMax = winner->speedMultiplierAtMax;
				behavior.minimumSpeed = winner->minimumSpeed;
				behavior.continuousRateFraction = winner->continuousRateFraction;
			}
			return behavior;
		}

		bool SameModifier(const sas::AttributeModifier& a, const sas::AttributeModifier& b)
		{
			return a.attributeId == b.attributeId && a.operation == b.operation &&
				a.magnitude == b.magnitude && a.priority == b.priority;
		}
	}

	ShipModuleApplier::ShipModuleApplier(ShipRuntime& shipRuntime, LightYearsAbilitySystemComponent& abilities, ModuleTriggerRuntime& triggerRuntime, CombatRuntime& combatRuntime)
		: mShipRuntime{ shipRuntime }
		, mAbilities{ abilities }
		, mCombatRuntime{ combatRuntime }
		, mAbilityRuleApplier{ abilities }
		, mTriggerBuffRuleApplier{ triggerRuntime }
		, mDamageRuleApplier{ combatRuntime }
		, mCooldownRuleApplier{ abilities }
		, mReactiveRuntime{ abilities }
		, mShieldReflectionRuntime{ combatRuntime }
		, mGeneratedRuntime{ abilities }
		, mOwnerBurstRuntime{ abilities }
		, mTargetSlowRuntime{ abilities }
		, mShieldRuptureRuntime{ abilities }
		, mRunCounterRuntime{ combatRuntime }
		, mReviveBonus{ abilities }
		, mEmergencyRuntime{ abilities, combatRuntime }
		, mEpicOffenseRuntime{ abilities, combatRuntime }
		, mEpicUtilityMovementRuntime{ abilities, combatRuntime }
		, mLegendarySecondHardpointRuntime{ abilities, combatRuntime }
		, mLegendaryMeltdownBarrelRuntime{ abilities, combatRuntime }
		, mLegendaryPrimedArsenalRuntime{ abilities, combatRuntime }
		, mLegendaryReactionCoreRuntime{ abilities, combatRuntime }
		, mLegendaryNeuralHijackerRuntime{ abilities, combatRuntime }
		, mLegendaryHardlightHullRuntime{ abilities, combatRuntime }
		, mLegendaryBulwarkPulseRuntime{ abilities, combatRuntime }
		, mLegendaryParasiteHullRuntime{ abilities, combatRuntime }
		, mLegendaryBlinkDriveRuntime{ abilities, combatRuntime }
		, mLegendaryIonCometRuntime{ abilities, combatRuntime }
		, mLegendaryTwinRackRuntime{ abilities, combatRuntime }
		, mLegendaryUnderdogCoreRuntime{ abilities, combatRuntime }
	{
		// The applier and the ability system component belong to the same ship and the
		// applier is destroyed first, so clearing the gate here leaves no dangling pointer.
		mAbilities.SetModuleSystemGate(this);
		mAbilityGrantedHandle = mAbilities.onAbilityGranted.BindAction(this, &ShipModuleApplier::OnLoadoutAbilityChanged);
		mAbilityRemovedHandle = mAbilities.onAbilityRemoved.BindAction(this, &ShipModuleApplier::OnLoadoutAbilityChanged);
		mAbilityChangedHandle = mAbilities.onAbilityChanged.BindAction(this, &ShipModuleApplier::OnLoadoutAbilityChanged);
		mAbilitiesClearedHandle = mAbilities.onAbilitiesCleared.BindAction(this, &ShipModuleApplier::OnLoadoutAbilitiesCleared);
		mReactiveRuntime.SetAbilityUseObserver([this](const std::string& abilityId, const List<GameplayTag>& tags) { OnOwnerAbilityUse(abilityId, tags); });
		mReactiveRuntime.SetKillObserver([this](sas::AbilitySlot slot, ControlTargetClass targetClass) { OnOwnerKill(slot, targetClass); });
	}

	ShipModuleApplier::~ShipModuleApplier()
	{
		// Removing modifiers broadcasts onAttributeChanged. During ship teardown this is safe
		// only because SpaceShip binds its listeners through weak_ptr and ShipRuntime outlives
		// this member; a raw `this` listener on a later PlayerSpaceShip member would dangle.
		mShuttingDown = true;
		mAbilities.onAbilityGranted.UnbindAction(mAbilityGrantedHandle);
		mAbilities.onAbilityRemoved.UnbindAction(mAbilityRemovedHandle);
		mAbilities.onAbilityChanged.UnbindAction(mAbilityChangedHandle);
		mAbilities.onAbilitiesCleared.UnbindAction(mAbilitiesClearedHandle);
		mAbilities.SetModuleSystemGate(nullptr);
		mCombatRuntime.SetModuleStatusRules({});
		mCombatRuntime.SetTargetStatusSnapshotRequested(false);
		mReactiveRuntime.Replace({}, {});
		mReactiveRuntime.ReplaceCharge({});
		mReactiveRuntime.ReplaceConditionalMovement({});
		mShieldReflectionRuntime.Replace({});
		mGeneratedRuntime.Replace({});
		mOwnerBurstRuntime.Replace({});
		mTargetSlowRuntime.Replace({});
		mShieldRuptureRuntime.Replace({});
		mRunCounterRuntime.Clear();
		try { mReviveBonus.Reset(); } catch (...) {}
		mEmergencyRuntime.Clear();
		mEpicOffenseRuntime.Shutdown();
		mLegendarySecondHardpointRuntime.Clear();
		mLegendaryMeltdownBarrelRuntime.Clear();
		mLegendaryPrimedArsenalRuntime.Clear();
		mLegendaryReactionCoreRuntime.Clear();
		mLegendaryNeuralHijackerRuntime.Clear();
		mLegendaryHardlightHullRuntime.Clear();
		mLegendaryBulwarkPulseRuntime.Clear();
		mLegendaryParasiteHullRuntime.Clear();
		mLegendaryBlinkDriveRuntime.Clear();
		mLegendaryIonCometRuntime.Clear();
		mLegendaryTwinRackRuntime.Clear();
		mLegendaryUnderdogCoreRuntime.Clear();
		mTriggerBuffRuleApplier.Reset();
		mAbilityRuleApplier.Reset();
		mCooldownRuleApplier.Reset();
		mDamageRuleApplier.Reset();

		for (const OwnedChargeBonus& owned : mChargeBonuses)
		{
			try { mAbilities.ClearAbilityChargeBonus(owned.slot); }
			catch (...) {}
		}
		mChargeBonuses.clear();
		List<OwnedModifier> ownerHandles = std::move(mOwnerHandles);
		List<OwnedModifier> shipHandles = std::move(mShipHandles);
		List<OwnedPrimaryModifier> primaryHandles = std::move(mPrimaryHandles);
		mOwnerHandles.clear();
		mShipHandles.clear();
		mPrimaryHandles.clear();
		LightYearsAbilitySystemComponent::ModuleAttributeChangeScope moduleScope{ mAbilities };
		for (const OwnedModifier& owned : ownerHandles)
		{
			try { mAbilities.GetAttributes().RemoveModifier(owned.handle); }
			catch (...) {}
		}
		for (const OwnedModifier& owned : shipHandles)
		{
			try { mShipRuntime.GetAttributes().RemoveModifier(owned.handle); }
			catch (...) {}
		}
		for (const OwnedPrimaryModifier& owned : primaryHandles)
		{
			try { mAbilities.RemovePrimaryWeaponModifier(owned.handle); }
			catch (...) {}
		}
	}

	std::size_t ShipModuleApplier::GetOwnedHandleCount() const
	{
		return mOwnerHandles.size() + mShipHandles.size() + mPrimaryHandles.size() +
			mAbilityRuleApplier.GetOwnedHandleCount() + mCooldownRuleApplier.GetOwnedHandleCount() + mReactiveRuntime.GetOwnedHandleCount() + mReactiveRuntime.GetOwnedSustainedHandleCount() + mRunCounterRuntime.GetOwnedSourceCount() + mChargeBonuses.size();
	}

	void ShipModuleApplier::OnLoadoutAbilityChanged(sas::AbilityHandle handle)
	{
		(void)handle;
		OnLoadoutAbilitiesCleared();
	}

	void ShipModuleApplier::OnLoadoutAbilitiesCleared()
	{
		if (mShuttingDown || !mHasApplied) return;
		EpicOffenseEffects updated = mResolved.epicOffense;
		ResolveMonochromeCoreLoadout(updated, mAbilities);
		if (mMovementAbilityCount != mAbilities.CountLoadoutAbilitiesWithTag(GameplayTags::Ability::Movement) || updated.monochromeMultipliers != mResolved.epicOffense.monochromeMultipliers) Rebuild();
	}

	float ShipModuleApplier::GetAbilitySetMainEffectMultiplier(sas::AbilityHandle ability) const
	{
		if (!mHasApplied || mResolved.distinctFamilyMainEffectBonus <= 0.f || !mAbilities.HasDistinctLoadoutFamilies(ability)) return 1.f;
		return 1.f + mResolved.distinctFamilyMainEffectBonus;
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
		if (mShuttingDown) return false;
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
		mSideFeatureLookup = rules.sideFeatureLookup;
		mAppliedRevision = loadout.GetRevision();
		Rebuild();
		mHasApplied = true;
		return true;
	}

	void ShipModuleApplier::Tick(float deltaSeconds)
	{
		if (!mShuttingDown) mReviveBonus.Tick(deltaSeconds);
		if (!mShuttingDown)
		{
			const auto* ship = dynamic_cast<const SpaceShip*>(&mAbilities.GetOwner());
			mEmergencyRuntime.Tick(deltaSeconds, ship ? &ship->GetHealthComponent() : nullptr);
		}
		if (!mShuttingDown) mReactiveRuntime.Tick(deltaSeconds);
		if (!mShuttingDown) mShieldReflectionRuntime.Tick(deltaSeconds);
		if (!mShuttingDown) mGeneratedRuntime.Tick(deltaSeconds);
		if (!mShuttingDown) mOwnerBurstRuntime.Tick(deltaSeconds);
		if (!mShuttingDown) mShieldRuptureRuntime.Tick(deltaSeconds);
		if (!mShuttingDown) mEpicOffenseRuntime.Tick(deltaSeconds);
		if (!mShuttingDown) mEpicUtilityMovementRuntime.Tick(deltaSeconds);
		if (!mShuttingDown) mLegendarySecondHardpointRuntime.Tick(deltaSeconds);
		if (!mShuttingDown) mLegendaryMeltdownBarrelRuntime.Tick(deltaSeconds);
		if (!mShuttingDown) mLegendaryPrimedArsenalRuntime.Tick(deltaSeconds);
		if (!mShuttingDown) mLegendaryReactionCoreRuntime.Tick(deltaSeconds);
		if (!mShuttingDown) mLegendaryNeuralHijackerRuntime.Tick(deltaSeconds);
		if (!mShuttingDown) mLegendaryHardlightHullRuntime.Tick(deltaSeconds);
		if (!mShuttingDown) mLegendaryBulwarkPulseRuntime.Tick(deltaSeconds);
		if (!mShuttingDown) mLegendaryParasiteHullRuntime.Tick(deltaSeconds);
		if (!mShuttingDown) mLegendaryBlinkDriveRuntime.Tick(deltaSeconds);
		if (!mShuttingDown) mLegendaryIonCometRuntime.Tick(deltaSeconds);
		if (!mShuttingDown) mLegendaryTwinRackRuntime.Tick(deltaSeconds);
		if (!mShuttingDown) mLegendaryUnderdogCoreRuntime.Tick(deltaSeconds);
		if (mShuttingDown || !mHasApplied || mReboots.empty() || deltaSeconds <= 0.f) return;

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

	void ShipModuleApplier::OnShipLevelChanged()
	{
		if (mShuttingDown || !mHasApplied || mResolvedShipLevel == mShipRuntime.GetShipLevel()) return;
		// A full rebuild resets transient module runtimes, so only loadouts with a ship-level term pay for it.
		if (!mResolved.usesShipLevel) { mResolvedShipLevel = mShipRuntime.GetShipLevel(); return; }
		Rebuild();
	}

	void ShipModuleApplier::Rebuild()
	{
		mMovementAbilityCount = mAbilities.CountLoadoutAbilitiesWithTag(GameplayTags::Ability::Movement);
		List<ModulePlacement> active;
		active.reserve(mPlacements.size());
		for (const ModulePlacement& placement : mPlacements)
		{
			if (!IsRebooting(placement.instanceId)) active.push_back(placement);
		}

		const ModuleRulesContext rules{ mSchematic, mLookup, mZoneTable, mSideFeatureLookup, [this](const GameplayTag& family) { return mAbilities.CountLoadoutAbilitiesWithTag(family); }, mShipRuntime.GetShipLevel() };
		mResolved = ModuleResolver::Resolve(rules, active, mPlacements);
		mResolvedShipLevel = rules.shipLevel;
		ResolveMonochromeCoreLoadout(mResolved.epicOffense, mAbilities);
		mDamageRuleApplier.Replace(mResolved.damageRules);
		mCombatRuntime.SetModuleStatusRules(mResolved.statusRules);
		mCombatRuntime.SetEpicOffenseEffects(mResolved.epicOffense);
		mEpicOffenseRuntime.Replace();

		// Modifiers that stay identical keep their handles and new ones are added before
		// stale ones are removed, so a reapply never dips a max value below both the old
		// and the new result; the current shield and afterburner energy are not clipped
		// by an unrelated change.
		{
			LightYearsAbilitySystemComponent::ModuleAttributeChangeScope scope{ mAbilities };
			SyncModifiers(mAbilities.GetAttributes(), mResolved.ownerModifiers, mOwnerHandles);
			SyncModifiers(mShipRuntime.GetAttributes(), mResolved.shipModifiers, mShipHandles);
			mLegendaryHardlightHullRuntime.Replace(mResolved.legendaryRules.hardlightHulls);
			if (auto* ship = dynamic_cast<SpaceShip*>(&mAbilities.GetOwner())) ship->RefreshMovementAttributesFromRuntime();
		}
		SyncPrimaryWeaponModifiers();
		mAbilities.SetPrimaryMountProfile(mResolved.primaryMount);
		mAbilityRuleApplier.Replace(mResolved.abilityRules);
		mCooldownRuleApplier.Replace(mResolved.cooldownRules);
		mTriggerBuffRuleApplier.Replace(mResolved.triggerBuffs);
		mReactiveRuntime.Replace(mResolved.eventActions, mResolved.ownerAttributeRules, mResolved.movementFadeRules);
		mReactiveRuntime.ReplaceCooldownRecovery(mResolved.cooldownRecoveryRules);
		if (auto* ship = dynamic_cast<SpaceShip*>(&mAbilities.GetOwner())) ship->GetDeferredDamageRuntime().SetRule(mResolved.deferredDamage);
	mReactiveRuntime.ReplaceSustainedSpeed(mResolved.sustainedSpeedRules);
		mShieldReflectionRuntime.Replace(mResolved.shieldReflections);
		SyncChargeBonuses(mResolved.chargeRules);
		mReactiveRuntime.ReplaceCharge(mResolved.chargeRules);
		mReactiveRuntime.ReplaceConditionalMovement(mResolved.conditionalMovementRules);
		if (auto* ship = dynamic_cast<SpaceShip*>(&mAbilities.GetOwner()))
		{
			ShieldModuleBehavior shieldBehavior = ResolveShieldBehavior(mResolved.shieldRules);
			ApplyShieldBatteryBehavior(mResolved.shieldBatteryRules, shieldBehavior);
			ship->GetShieldComponent().SetModuleBehavior(shieldBehavior);
		}
		const ModuleMovementBehavior movement = ResolveMovementBehavior(mResolved.movementRules);
		if (auto* ship = dynamic_cast<SpaceShip*>(&mAbilities.GetOwner())) ship->GetMovementComponent().SetModuleMovementRule(movement.thrustBonus, movement.equalizeDirections);
		mGeneratedRuntime.Replace(mResolved.generatedRules);
		mOwnerBurstRuntime.Replace(mResolved.ownerBurstRules);
		mEpicUtilityMovementRuntime.Replace(mResolved.epicRules);
		mLegendarySecondHardpointRuntime.Replace(mResolved.legendaryRules.secondHardpoints);
		// The weapon chosen for the placement (Player::ChooseSecondaryWeapon) becomes the runtime's second weapon.
		for (const ResolvedModuleLegendarySecondHardpointRule& rule : mResolved.legendaryRules.secondHardpoints)
		{
			const auto placement = std::find_if(mPlacements.begin(), mPlacements.end(), [&rule](const ModulePlacement& candidate) { return candidate.instanceId == rule.instanceId; });
			if (placement != mPlacements.end() && !placement->secondaryWeaponId.empty() && mLegendarySecondHardpointRuntime.GetSecondWeaponId() != placement->secondaryWeaponId)
				mLegendarySecondHardpointRuntime.AssignSecondWeapon(placement->secondaryWeaponId);
		}
		mLegendaryMeltdownBarrelRuntime.Replace(mResolved.legendaryRules.meltdownBarrels);
		mLegendaryPrimedArsenalRuntime.Replace(mResolved.legendaryRules.primedArsenals);
		mLegendaryReactionCoreRuntime.Replace(mResolved.legendaryRules.reactionCores);
		mLegendaryNeuralHijackerRuntime.Replace(mResolved.legendaryRules.neuralHijackers);
		mLegendaryBulwarkPulseRuntime.Replace(mResolved.legendaryRules.bulwarkPulses);
		mLegendaryParasiteHullRuntime.Replace(mResolved.legendaryRules.parasiteHulls);
		mLegendaryBlinkDriveRuntime.Replace(mResolved.legendaryRules.blinkDrives);
		mLegendaryIonCometRuntime.Replace(mResolved.legendaryRules.ionComets);
		mLegendaryTwinRackRuntime.Replace(mResolved.legendaryRules.twinRacks);
		mLegendaryUnderdogCoreRuntime.Replace(mResolved.legendaryRules.underdogCores);
		mTargetSlowRuntime.Replace(mResolved.targetSlowRules);
		mShieldRuptureRuntime.Replace(mResolved.shieldRuptureRules);
		if (auto* ship = dynamic_cast<SpaceShip*>(&mAbilities.GetOwner())) mRunCounterRuntime.Replace(active, mLookup, mResolved, &ship->GetRuntimeModifiers());
		else mRunCounterRuntime.Replace(active, mLookup, mResolved, nullptr);
		mRunCounterRuntime.ReplaceKillSources(mResolved.killSources);
		if (auto* ship = dynamic_cast<SpaceShip*>(&mAbilities.GetOwner())) mEmergencyRuntime.Replace(active, rules, mResolved, &ship->GetRuntimeModifiers());
		else mEmergencyRuntime.Replace(active, rules, mResolved, nullptr);
		mGeneratedRuntime.SetRunCounterDamageMultiplier(mRunCounterRuntime.GetDamageMultiplier());
		bool needsStatusSnapshot = !mResolved.generatedRules.killExplosions.empty();
		for (const auto& resolved : mResolved.generatedRules.ricochets) needsStatusSnapshot = needsStatusSnapshot || resolved.rule.transferStatus;
		mCombatRuntime.SetTargetStatusSnapshotRequested(needsStatusSnapshot);

		mEnabledSystems = mResolved.enabledSystems;
		mShipRuntime.SetSystemEnabled(ShipSystem::Shield, IsSystemEnabled(GameplayTags::Module::System::Shield));
		mShipRuntime.SetSystemEnabled(ShipSystem::Afterburner, IsSystemEnabled(GameplayTags::Module::System::Afterburner));
		mAbilities.CancelAbilitiesUnavailableByModules();
	}

	void ShipModuleApplier::OnOwnerAbilityUse(const std::string& abilityId, const List<GameplayTag>& tags)
	{
		if (mShuttingDown || !mHasApplied) return;
		mEpicOffenseRuntime.OnOwnerAbilityUse(tags);
		mEpicUtilityMovementRuntime.OnAbilityUse(abilityId, tags);
		bool changed = false;
		for (auto& placement : mPlacements)
		{
			if (IsRebooting(placement.instanceId)) continue;
			const auto* module = mLookup(placement.moduleId); if (!module) continue;
			const int level = placement.level + ModuleResolver::FindZoneLevelBonus(mResolved, placement.instanceId);
			for (const auto& rule : module->sizeVariants[static_cast<std::size_t>(placement.variantIndex)].runCounterRules)
			{
				if (rule.source != ModuleRunCounterSource::AbilityUses || !ModuleTriggerEventTagsMatch({ rule.requiredFamily }, tags)) continue;
				const double limit = rule.armorCapByLevel.ValueAtLevel(rule.armorCap, level) / rule.armorPerStack * rule.unitsPerStack;
				const double next = std::min(limit, placement.runCounter + 1.0), delta = next - placement.runCounter;
				if (delta <= 0.0) continue;
				placement.runCounter = next; if (mRunCounterCreditSink) mRunCounterCreditSink(placement.instanceId, delta); changed = true;
			}
		}
		if (changed) Rebuild();
	}
	void ShipModuleApplier::OnOwnerKill(sas::AbilitySlot slot, ControlTargetClass targetClass)
	{
		if (mShuttingDown || !mHasApplied) return;
		bool credited = false;
		for (const ResolvedModuleKillRule& kill : mResolved.killRules)
		{
			const bool adjacent = std::any_of(kill.adjacentAbilitySystems.begin(), kill.adjacentAbilitySystems.end(),
				[slot](const GameplayTag& system) { return AbilitySlotForModuleSystem(system) == slot; });
			const double weight = static_cast<double>(kill.weights.ForClass(targetClass));
			if (!adjacent || !(weight > 0.0)) continue;
			const auto placement = std::find_if(mPlacements.begin(), mPlacements.end(), [&kill](const ModulePlacement& owned) { return owned.instanceId == kill.instanceId; });
			if (placement == mPlacements.end()) continue;
			const double normalBonus = weight * static_cast<double>(kill.damagePerKill);
			const double bonus = targetClass == ControlTargetClass::Boss ? (kill.bossDamagePerKill ? static_cast<double>(*kill.bossDamagePerKill) : normalBonus) :
				(targetClass == ControlTargetClass::Elite || targetClass == ControlTargetClass::MiniBoss) ? (kill.eliteDamagePerKill ? static_cast<double>(*kill.eliteDamagePerKill) : normalBonus) : normalBonus;
			placement->killStacks += weight;
			placement->killDamageBonus += bonus;
			if (mKillCreditSink) mKillCreditSink(kill.instanceId, weight, bonus);
			credited = true;
		}
		if (credited) RefreshKillEffects();
	}

	// Kill credit changes only the kill-derived effects, so only their appliers are re-synced; the rest of the resolved state
	// is a pure function of the unchanged layout and stays as it is.
	void ShipModuleApplier::RefreshKillEffects()
	{
		List<ModulePlacement> active;
		active.reserve(mPlacements.size());
		for (const ModulePlacement& placement : mPlacements)
		{
			if (!IsRebooting(placement.instanceId)) active.push_back(placement);
		}
		const ModuleRulesContext rules{ mSchematic, mLookup, mZoneTable, mSideFeatureLookup, [this](const GameplayTag& family) { return mAbilities.CountLoadoutAbilitiesWithTag(family); }, mShipRuntime.GetShipLevel() };
		mResolved = ModuleResolver::Resolve(rules, active, mPlacements);
		mResolvedShipLevel = rules.shipLevel;
		mCooldownRuleApplier.Replace(mResolved.cooldownRules);
		mRunCounterRuntime.ReplaceKillSources(mResolved.killSources);
	}

	bool ShipModuleApplier::TryPreventOwnerDeath(SpaceShip& owner, const DamageContext& context)
	{
		if (mShuttingDown) return false;
		if (mLegendaryParasiteHullRuntime.TryPossess(context)) return true;
		return TryReviveOwner(owner);
	}

	bool ShipModuleApplier::TryReviveOwner(SpaceShip& owner)
	{
		if (mShuttingDown || !mHasApplied) return false;
		for (ModulePlacement& placement : mPlacements)
		{
			if (IsRebooting(placement.instanceId)) continue;
			const ModuleDefinition* module = mLookup ? mLookup(placement.moduleId) : nullptr;
			if (!module || placement.variantIndex < 0 || static_cast<std::size_t>(placement.variantIndex) >= module->sizeVariants.size()) continue;
			const List<ModuleReviveRule>& reviveRules = module->sizeVariants[static_cast<std::size_t>(placement.variantIndex)].reviveRules;
			if (reviveRules.empty()) continue;
			const ModuleReviveRule& rule = reviveRules.front();
			const int level = placement.level + ModuleResolver::FindZoneLevelBonus(mResolved, placement.instanceId);
			if (placement.reviveChargesSpent >= ResolveEpicLevelValue(rule.chargesByLevel, 0, level)) continue;
			HealthComponent& health = owner.GetHealthComponent();
			++placement.reviveChargesSpent;
			if (mReviveSpentSink) mReviveSpentSink(placement.instanceId);
			health.ChangeHealth(ResolveEpicLevelValue(rule.healthFractionByLevel, 0.f, level) * health.GetMaxHealth() - health.GetHealth());
			mReviveBonus.Start(rule.bonusSeconds, rule.bonusMultiplier);
			return true;
		}
		return false;
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

	void ShipModuleApplier::SyncChargeBonuses(const List<ResolvedModuleChargeRule>& rules)
	{
		// Two placements that credit one ability never add up: the ability takes the largest bonus.
		List<OwnedChargeBonus> desired;
		for (const ResolvedModuleChargeRule& rule : rules)
		{
			if (rule.chargeBonus <= 0) continue;
			for (const sas::AbilitySlot slot : rule.slots)
			{
				const auto found = std::find_if(desired.begin(), desired.end(), [slot](const OwnedChargeBonus& owned) { return owned.slot == slot; });
				if (found == desired.end()) desired.push_back({ slot, rule.chargeBonus });
				else found->bonus = std::max(found->bonus, rule.chargeBonus);
			}
		}
		for (const OwnedChargeBonus& owned : mChargeBonuses)
		{
			const bool stillDesired = std::any_of(desired.begin(), desired.end(), [&owned](const OwnedChargeBonus& want) { return want.slot == owned.slot; });
			if (!stillDesired) mAbilities.ClearAbilityChargeBonus(owned.slot);
		}
		// Setting an unchanged bonus is a no-op in the ability, so the existing charges and timer keep their state.
		for (const OwnedChargeBonus& want : desired) mAbilities.SetAbilityChargeBonus(want.slot, want.bonus);
		mChargeBonuses = std::move(desired);
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

}
