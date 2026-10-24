#pragma once

#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/module/ModuleResolver.h"

#include <algorithm>
#include <cstdint>

namespace vega
{
	// Resolves family-counted Epic stats (EpicFamilyStatRule) from the installed layout. Runs once per Resolve, after zone levels.
	void ApplyEpicFamilyStatRules(ResolvedModuleEffects& effects, const ModuleRulesContext& rules, const List<ModulePlacement>& activePlacements);

	// Twin Reactor: the value of `id` that makes it equal to max(AP, EP) + bonus. The comparison excludes only the PowerEqualize
	// handles of the same placement, so the placement's own bonus never feeds back, while other owner modifiers still count.
	template<typename Attributes, typename States>
	float EpicPowerEqualizeAmount(const Attributes& attributes, const States& states, const sas::AttributeId& id, std::uint32_t instanceId, float bonus)
	{
		List<sas::AttributeModifierHandle> own;
		for (const auto& state : states)
			if (state.handle.IsValid() && state.resolved.instanceId == instanceId && state.resolved.rule.source == ModuleOwnerAttributeSource::PowerEqualize) own.push_back(state.handle);
		const float attack = attributes.GetCurrentValueExcludingModifiers(OwnerAttributeIds::AttackPower, own);
		const float energy = attributes.GetCurrentValueExcludingModifiers(OwnerAttributeIds::EnergyPower, own);
		return std::max(attack, energy) + bonus - attributes.GetCurrentValueExcludingModifiers(id, own);
	}
}
