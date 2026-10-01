#include "gameplay/module/ShipModuleApplier.h"

#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/tags/GameplayTags.h"

#include <algorithm>
#include <utility>

namespace ly
{
	ShipModuleApplier::ShipModuleApplier(ShipRuntime& shipRuntime, LightYearsAbilitySystemComponent& abilities)
		: mShipRuntime{ shipRuntime }
		, mAbilities{ abilities }
	{
	}

	bool ShipModuleApplier::IsSystemEnabled(const GameplayTag& systemTag) const
	{
		return std::find(mEnabledSystems.begin(), mEnabledSystems.end(), systemTag) != mEnabledSystems.end();
	}

	bool ShipModuleApplier::Apply(const ModuleLoadout& loadout, const ModuleRulesContext& rules)
	{
		if (mHasApplied && mAppliedRevision == loadout.GetRevision()) return false;

		RemoveAll();
		mResolved = ModuleResolver::Resolve(rules, loadout.GetPlacements());

		sas::AttributeSystem& ownerAttributes = mAbilities.GetAttributes();
		for (const sas::AttributeModifier& modifier : mResolved.ownerModifiers)
		{
			const sas::AttributeModifierHandle handle = ownerAttributes.AddModifier(modifier);
			if (handle.IsValid()) mOwnerHandles.push_back(handle);
		}
		sas::AttributeSystem& shipAttributes = mShipRuntime.GetAttributes();
		for (const sas::AttributeModifier& modifier : mResolved.shipModifiers)
		{
			const sas::AttributeModifierHandle handle = shipAttributes.AddModifier(modifier);
			if (handle.IsValid()) mShipHandles.push_back(handle);
		}
		for (const ScopedAbilityRule& rule : mResolved.abilityRules)
		{
			mScopedRuleHandles.push_back(mAbilities.AddScopedAbilityRule(rule));
		}

		mEnabledSystems = mResolved.enabledSystems;
		mShipRuntime.SetSystemEnabled(ShipSystem::Shield, IsSystemEnabled(GameplayTags::Module::System::Shield));
		mShipRuntime.SetSystemEnabled(ShipSystem::Afterburner, IsSystemEnabled(GameplayTags::Module::System::Afterburner));

		mAppliedRevision = loadout.GetRevision();
		mHasApplied = true;
		return true;
	}

	void ShipModuleApplier::RemoveAll()
	{
		sas::AttributeSystem& ownerAttributes = mAbilities.GetAttributes();
		for (const sas::AttributeModifierHandle handle : mOwnerHandles) ownerAttributes.RemoveModifier(handle);
		mOwnerHandles.clear();
		sas::AttributeSystem& shipAttributes = mShipRuntime.GetAttributes();
		for (const sas::AttributeModifierHandle handle : mShipHandles) shipAttributes.RemoveModifier(handle);
		mShipHandles.clear();
		for (const std::size_t handle : mScopedRuleHandles) mAbilities.RemoveScopedAbilityRule(handle);
		mScopedRuleHandles.clear();
	}
}
