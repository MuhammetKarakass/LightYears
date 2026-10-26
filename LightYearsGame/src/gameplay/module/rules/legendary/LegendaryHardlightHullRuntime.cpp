#include "gameplay/module/rules/legendary/LegendaryHardlightHullRuntime.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/attributes/AttributeIds.h"
#include "spaceShip/SpaceShip.h"
#include <algorithm>
#include <limits>

namespace vega
{
	LegendaryHardlightHullRuntime::LegendaryHardlightHullRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime) : mAbilities{ abilities }, mCombatRuntime{ combatRuntime } {}
	LegendaryHardlightHullRuntime::~LegendaryHardlightHullRuntime() { Clear(); }

	void LegendaryHardlightHullRuntime::Replace(const List<ResolvedModuleLegendaryHardlightHullRule>& rules)
	{
		mRules = rules;
		if (mRules.empty()) Clear();
		else Refresh();
	}

	void LegendaryHardlightHullRuntime::Tick(float deltaSeconds)
	{
		(void)deltaSeconds;
		Refresh();
	}

	void LegendaryHardlightHullRuntime::Refresh()
	{
		if (mRefreshing || mRules.empty()) return;
		auto* ship = dynamic_cast<SpaceShip*>(&mAbilities.GetOwner());
		if (!ship) return;
		mRefreshing = true;
		struct Reset { bool& flag; ~Reset() { flag = false; } } reset{ mRefreshing };
		LightYearsAbilitySystemComponent::ModuleAttributeChangeScope scope{ mAbilities };
		auto& owner = mAbilities.GetAttributes();
		auto& stats = ship->GetShipRuntime().GetAttributes();
		const auto& rule = *std::max_element(mRules.begin(), mRules.end(), [](const auto& a, const auto& b) { return a.healthToShieldFraction < b.healthToShieldFraction; });
		const float health = owner.GetCurrentValueExcludingModifiers(OwnerAttributeIds::MaxHealth, { mHealthHandle });
		const float shield = stats.GetCurrentValueExcludingModifiers(ShipAttributeIds::MaxShield, { mShieldHandle }) + rule.healthToShieldFraction * health;
		const auto replace = [](sas::AttributeSystem& attributes, const sas::AttributeId& id, float value, sas::AttributeModifierHandle& handle)
		{
			const auto previous = handle;
			attributes.AddModifier({ id, sas::AttributeModifierOperation::Override, value, std::numeric_limits<int>::max() }, handle);
			if (previous.IsValid()) attributes.RemoveModifier(previous);
		};
		if (!mHealthHandle.IsValid() || mConvertedHealth != rule.maxHealth)
		{
			replace(owner, OwnerAttributeIds::MaxHealth, rule.maxHealth, mHealthHandle);
			mConvertedHealth = rule.maxHealth;
		}
		if (!mShieldHandle.IsValid() || mConvertedShield != shield)
		{
			replace(stats, ShipAttributeIds::MaxShield, shield, mShieldHandle);
			mConvertedShield = shield;
		}
		ship->GetHealthComponent().SetMaxHealth(owner.GetCurrentValue(OwnerAttributeIds::MaxHealth), true);
		const bool emptyPool = ship->GetShieldComponent().GetMaxShield() <= 0.f;
		ship->GetShieldComponent().SetMaxShield(stats.GetCurrentValue(ShipAttributeIds::MaxShield), true);
		if (emptyPool) ship->GetShieldComponent().ChangeShield(-ship->GetShieldComponent().GetMaxShield());
	}

	void LegendaryHardlightHullRuntime::Clear()
	{
		mRules.clear();
		if (!mHealthHandle.IsValid() && !mShieldHandle.IsValid()) return;
		LightYearsAbilitySystemComponent::ModuleAttributeChangeScope scope{ mAbilities };
		auto* ship = dynamic_cast<SpaceShip*>(&mAbilities.GetOwner());
		if (mHealthHandle.IsValid()) mAbilities.GetAttributes().RemoveModifier(mHealthHandle);
		if (ship && mShieldHandle.IsValid()) ship->GetShipRuntime().GetAttributes().RemoveModifier(mShieldHandle);
		mHealthHandle = {};
		mShieldHandle = {};
		if (ship)
		{
			ship->GetHealthComponent().SetMaxHealth(mAbilities.GetAttributes().GetCurrentValue(OwnerAttributeIds::MaxHealth), true);
			ship->GetShieldComponent().SetMaxShield(ship->GetShipRuntime().GetAttributes().GetCurrentValue(ShipAttributeIds::MaxShield), true);
		}
	}
}
