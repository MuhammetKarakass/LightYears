#include "gameplay/module/rules/legendary/LegendaryPrimedArsenalRuntime.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ability/glacialPressure/GlacialPressureContracts.h"
#include "gameplay/ability/executionDrive/ExecutionDriveContracts.h"
#include "gameplay/ability/overdriveCore/OverdriveCoreContracts.h"
#include "gameplay/ability/combatSentry/CombatSentryContracts.h"
#include "gameplay/ability/nanoPlague/NanoPlagueContracts.h"
#include <algorithm>
#include <cmath>

namespace vega
{
	LegendaryPrimedArsenalRuntime::LegendaryPrimedArsenalRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime) : mAbilities{ abilities }, mCombatRuntime{ combatRuntime } {}
	LegendaryPrimedArsenalRuntime::~LegendaryPrimedArsenalRuntime() { Clear(); }

	void LegendaryPrimedArsenalRuntime::Replace(const List<ResolvedModuleLegendaryPrimedArsenalRule>& rules)
	{
		const bool sameSources = rules.size() == mRules.size() && std::equal(rules.begin(), rules.end(), mRules.begin(), [](const auto& a, const auto& b) { return a.instanceId == b.instanceId; });
		if (!sameSources) mReadySeconds.clear();
		mRules = rules;
	}

	void LegendaryPrimedArsenalRuntime::Tick(float deltaSeconds)
	{
		if (mRules.empty() || !std::isfinite(deltaSeconds) || deltaSeconds <= 0.f) return;
		float window = 0.f;
		for (const auto& rule : mRules) window = std::max(window, rule.readyWindowSeconds);
		std::map<sas::AbilityHandle, float> next;
		for (const auto& snapshot : mAbilities.BuildAbilitySnapshots())
		{
			const auto* ability = mAbilities.GetAbility(snapshot.handle);
			if (!ability || !sas::IsLoadoutAbilitySlot(snapshot.slot)) continue;
			const auto& tags = ability->GetDefinition().abilityTags;
			if (std::none_of(tags.begin(), tags.end(), [](const auto& tag) { return tag.MatchesTag(GameplayTags::Ability::Offense); })) continue;
			float seconds = mReadySeconds[snapshot.handle];
			if (!snapshot.active && snapshot.cooldownRemaining <= 0.f && (snapshot.chargeCapacity <= 0 || snapshot.charges == snapshot.chargeCapacity)) seconds += deltaSeconds;
			next.emplace(snapshot.handle, std::min(seconds, window));
		}
		mReadySeconds = std::move(next);
	}

	float LegendaryPrimedArsenalRuntime::GetMultiplier(sas::AbilityHandle ability) const
	{
		const auto found = mReadySeconds.find(ability);
		if (found == mReadySeconds.end()) return 1.f;
		float multiplier = 1.f;
		for (const auto& rule : mRules) multiplier = std::max(multiplier, 1.f + rule.powerPerReadySecond * std::min(found->second, rule.readyWindowSeconds));
		return multiplier;
	}

	void LegendaryPrimedArsenalRuntime::OnUseCommitted(sas::AbilityHandle ability)
	{
		mReadySeconds.erase(ability);
	}

	void LegendaryPrimedArsenalRuntime::Clear()
	{
		mRules.clear();
		mReadySeconds.clear();
	}

	bool LegendaryPrimedArsenalRuntime::IsMainOutput(const sas::AttributeId& attribute)
	{
		// The existing Cascade/Ultimate output list also includes movement and knockback strength.
		// Primed powers only damage and AP/AS amounts, including their explicit formula coefficients.
		for (const auto& output : { CommonAttributeIds::Damage, AbilityData::GlacialPressure::Attribute::InitialDamage,
			AbilityData::GlacialPressure::Attribute::CollisionDamage, AbilityData::GlacialPressure::Attribute::EnergyPowerInitialScale,
			AbilityData::GlacialPressure::Attribute::EnergyPowerCollisionScale, AbilityData::GlacialPressure::Attribute::MaxHealthCollisionScale,
			AbilityData::ExecutionDrive::Attribute::FlatAttackPowerBonus, AbilityData::ExecutionDrive::Attribute::AttackPowerScale,
			AbilityData::OverdriveCore::Attribute::AttackSpeedBoostBase, AbilityData::CombatSentry::Actor::Turret::BaseDamage,
			AbilityData::CombatSentry::Actor::Turret::OwnerAttackPowerScale, AbilityData::NanoPlague::Attribute::EnergyPowerTickScale })
			if (attribute == output) return true;
		return false;
	}
}
