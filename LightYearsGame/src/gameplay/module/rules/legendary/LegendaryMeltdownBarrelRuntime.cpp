#include "gameplay/module/rules/legendary/LegendaryMeltdownBarrelRuntime.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/module/generated/ModuleGeneratedDamage.h"
#include "gameplay/targeting/CombatantTargetQuery.h"
#include "gameplay/combat/Combatant.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "framework/Actor.h"
#include <algorithm>
#include <cmath>

namespace vega
{
	LegendaryMeltdownBarrelRuntime::LegendaryMeltdownBarrelRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime) : mAbilities{ abilities }, mCombatRuntime{ combatRuntime } {}
	LegendaryMeltdownBarrelRuntime::~LegendaryMeltdownBarrelRuntime() { Clear(); }

	void LegendaryMeltdownBarrelRuntime::Replace(const List<ResolvedModuleLegendaryMeltdownBarrelRule>& rules)
	{
		if (!mRules.empty() && !rules.empty() && mRules.front().instanceId == rules.front().instanceId)
		{
			mRules = rules;
			return;
		}
		Clear();
		mRules = rules;
		if (!mRules.empty()) mAbilities.SetMeltdownBarrelRuntime(this);
	}

	void LegendaryMeltdownBarrelRuntime::Tick(float deltaSeconds)
	{
		if (mRules.empty() || deltaSeconds <= 0.f) return;
		Actor& owner = mAbilities.GetOwner();
		// Snapshot the list: damage callbacks may uninstall or rebuild the module.
		const auto cycles = mCycles;
		for (const auto& weak : cycles)
		{
			const auto cycle = weak.lock();
			if (!cycle || !cycle->active) continue;
			if (cycle->startedDuringAbilityTick)
			{
				cycle->startedDuringAbilityTick = false;
				continue;
			}
			cycle->fireLockRemaining = std::max(0.f, cycle->fireLockRemaining - deltaSeconds);
			if (cycle->focusRemaining <= 0.f) continue;
			cycle->focusRemaining = std::max(0.f, cycle->focusRemaining - deltaSeconds);
			if (cycle->focusRemaining > 0.f) continue;
			const float damage = cycle->pendingDamage;
			cycle->pendingDamage = 0.f;
			if (!owner.GetWorld() || owner.GetIsPendingDestroy() || mRules.empty()) continue;
			const auto rule = mRules.front();
			const auto targets = targeting::FindOpposingCombatants(*owner.GetWorld(), owner, owner.GetActorLocation(), rule.novaRadius, false);
			for (const auto& target : targets)
			{
				if (!cycle->active || owner.GetIsPendingDestroy()) break;
				if (!target->GetIsPendingDestroy()) ApplyCombatDamage(*target, damage, &owner, { DamageTypeSchema::Thermal },
					MakeModuleGeneratedDamagePayload(0, DamageSourceScalingPolicy::Normal, rule.igniteStacks), {}, {}, DamageDeliveryType::Area, &owner);
			}
		}
		mCycles.erase(std::remove_if(mCycles.begin(), mCycles.end(), [](const auto& cycle) { return cycle.expired(); }), mCycles.end());
	}

	bool LegendaryMeltdownBarrelRuntime::Supports(const PrimaryWeaponDefinition& definition) const
	{
		return !mRules.empty() && (definition.magazine.has_value() ||
			std::find(definition.featureTypes.begin(), definition.featureTypes.end(), PrimaryWeaponFeatureType::Heat) != definition.featureTypes.end());
	}

	bool LegendaryMeltdownBarrelRuntime::CanFire(const PrimaryWeaponRuntimeState& state) const
	{
		return !state.meltdownCycle || !state.meltdownCycle->active || state.meltdownCycle->fireLockRemaining <= 0.f;
	}

	std::shared_ptr<MeltdownWeaponCycle> LegendaryMeltdownBarrelRuntime::EnsureCycle(const PrimaryWeaponExecutionContext& context, PrimaryWeaponRuntimeState& state)
	{
		if (!Supports(context.definition)) return {};
		if (!state.meltdownCycle || !state.meltdownCycle->active)
		{
			state.meltdownCycle = std::make_shared<MeltdownWeaponCycle>();
			if (context.definition.magazine)
				state.meltdownCycle->heat = GetHeatCapacity() * (1.f - static_cast<float>(state.magazineState.roundsRemaining) / context.definition.magazine->capacity);
			mCycles.push_back(state.meltdownCycle);
		}
		return state.meltdownCycle;
	}

	float LegendaryMeltdownBarrelRuntime::NormalDamage(const PrimaryWeaponExecutionContext& context) const
	{
		const auto* primary = mAbilities.GetAbility(sas::AbilitySlot::PrimaryFire);
		const auto* progression = primary && primary->GetDefinition().abilityId == context.definition.weaponId ? &primary->GetDefinition() : nullptr;
		const auto raw = [&](const sas::AttributeId& id, float fallback)
		{
			const auto* attribute = sas::FindAttribute(context.definition.attributes, id);
			if (!attribute) return fallback;
			float value = progression ? sas::CalculateModifiedAttributeValue(*attribute, progression->attributeModifiers) : attribute->currentValue;
			value = sas::CalculateModifiedAttributeValue(sas::GameplayAttribute{ id, value, attribute->minValue, attribute->maxValue }, context.definition.attributeModifiers);
			value = sas::ApplyAttributeScalings(value, id, context.definition.scalingRules, mAbilities.GetAttributes());
			if (progression) value = sas::ApplyAttributeScalings(value, id, progression->levelScalingRules, mAbilities.GetAttributes());
			return std::clamp(value, attribute->minValue, attribute->maxValue);
		};
		float count = 1.f;
		if (IsProjectileWeaponType(context.definition.weaponType))
		{
			const int pellets = std::max(1, static_cast<int>(std::round(raw(PrimaryWeaponSchema::Projectile::Shotgun::PelletCount, 1.f)))) +
				std::max(0, static_cast<int>(std::round(raw(PrimaryWeaponSchema::Projectile::Delivery::AdditionalProjectileCount, 0.f))));
			const int muzzles = context.definition.alternateMuzzles ? 1 : std::max(1, static_cast<int>(context.definition.muzzleDefinitions.size()));
			const float reduction = raw(PrimaryWeaponSchema::Projectile::Shotgun::DamageReductionPerAdditionalHit, 0.f);
			const float minimum = raw(PrimaryWeaponSchema::Projectile::Shotgun::MinimumDamageMultiplier, 1.f);
			count = 0.f;
			for (int i = 0; i < pellets * muzzles; ++i) count += std::max(minimum, 1.f - reduction * i);
		}
		return std::max(0.f, raw(CommonAttributeIds::Damage, 0.f) * count);
	}

	void LegendaryMeltdownBarrelRuntime::RecordTrigger(const PrimaryWeaponExecutionContext& context, PrimaryWeaponRuntimeState& state)
	{
		const auto cycle = EnsureCycle(context, state);
		if (!cycle) return;
		cycle->budget += NormalDamage(context);
		if (!context.definition.magazine) return;
		const auto& magazine = *context.definition.magazine;
		const int roundsPerShot = context.definition.alternateMuzzles ? 1 : std::max(1, static_cast<int>(context.definition.muzzleDefinitions.size()));
		const int shots = (magazine.capacity + roundsPerShot - 1) / roundsPerShot;
		cycle->heat = std::min(GetHeatCapacity(), cycle->heat + GetHeatCapacity() / shots);
		state.magazineState.roundsRemaining = std::clamp(static_cast<int>(std::ceil(magazine.capacity * (1.f - cycle->heat / GetHeatCapacity()) - 0.0001f)), 0, magazine.capacity);
		if (cycle->heat >= GetHeatCapacity() - 0.0001f)
		{
			cycle->heat = GetHeatCapacity();
			state.magazineState.roundsRemaining = 0;
			const float reload = PrimaryWeaponExecutionSystem::CalculateReloadDuration(magazine.baseReloadTime, context.owner);
			state.magazineState.reloadRemaining = reload;
			Overheat(context, state, reload);
		}
	}

	void LegendaryMeltdownBarrelRuntime::RecordBeam(const PrimaryWeaponExecutionContext& context, float heatBandMultiplier, float deltaSeconds)
	{
		if (!context.runtime || !context.owner.GetWorld()) return;
		if (const auto cycle = EnsureCycle(context, *context.runtime)) cycle->budget += NormalDamage(context) * heatBandMultiplier * deltaSeconds;
	}

	void LegendaryMeltdownBarrelRuntime::Overheat(const PrimaryWeaponExecutionContext& context, PrimaryWeaponRuntimeState& state, float naturalLock)
	{
		const auto cycle = EnsureCycle(context, state);
		if (!cycle || cycle->focusRemaining > 0.f) return;
		cycle->pendingDamage = cycle->budget * mRules.front().novaBudgetFraction;
		cycle->budget = 0.f;
		cycle->focusRemaining = mRules.front().focusSeconds;
		cycle->fireLockRemaining = std::max(naturalLock, cycle->focusRemaining);
		cycle->startedDuringAbilityTick = mAbilities.IsExecutingAbilityInstanceOperation();
		// Natural recovery stays on the existing scheduler; the module owns only the focus lock.
		state.RequestCooldown(naturalLock);
	}

	void LegendaryMeltdownBarrelRuntime::Cool(const PrimaryWeaponExecutionContext& context, PrimaryWeaponRuntimeState& state, float previousHeat, float newHeat)
	{
		if (!Supports(context.definition) || !state.meltdownCycle || !state.meltdownCycle->active) return;
		auto& cycle = *state.meltdownCycle;
		if (newHeat < previousHeat) cycle.budget = previousHeat > 0.f ? cycle.budget * newHeat / previousHeat : 0.f;
		cycle.heat = newHeat;
	}

	bool LegendaryMeltdownBarrelRuntime::CoolMagazine(const PrimaryWeaponExecutionContext& context, PrimaryWeaponRuntimeState& state, float deltaSeconds)
	{
		if (!context.definition.magazine || !Supports(context.definition)) return false;
		const auto cycle = EnsureCycle(context, state);
		const auto& magazine = *context.definition.magazine;
		const float reload = PrimaryWeaponExecutionSystem::CalculateReloadDuration(magazine.baseReloadTime, context.owner);
		const float cooledHeat = cycle->heat - GetHeatCapacity() * deltaSeconds / reload;
		const float heat = cooledHeat < 0.0001f ? 0.f : cooledHeat;
		Cool(context, state, cycle->heat, heat);
		state.magazineState.roundsRemaining = std::clamp(static_cast<int>(std::ceil(magazine.capacity * (1.f - heat / GetHeatCapacity()) - 0.0001f)), 0, magazine.capacity);
		if (heat <= 0.f) state.magazineState.shotsFired = 0;
		return true;
	}

	void LegendaryMeltdownBarrelRuntime::Clear()
	{
		for (const auto& weak : mCycles) if (const auto cycle = weak.lock()) { cycle->active = false; cycle->budget = 0.f; cycle->pendingDamage = 0.f; }
		mCycles.clear();
		if (mAbilities.GetMeltdownBarrelRuntime() == this) mAbilities.SetMeltdownBarrelRuntime(nullptr);
		mRules.clear();
	}
}
