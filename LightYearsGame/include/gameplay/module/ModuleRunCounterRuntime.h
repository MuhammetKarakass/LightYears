#pragma once

#include "framework/Core.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/combat/CombatRuntimeModifiers.h"
#include "gameplay/module/ModuleLoadout.h"
#include "gameplay/module/ModulePlacementRules.h"
#include "gameplay/module/ModuleResolver.h"
#include "gameplay/ship/ShipRuntimeModifiers.h"

#include <algorithm>
#include <cstddef>
#include <string>

namespace vega
{
	// Turns run counter stacks into live multipliers. Each (placement, rule) pair owns one source id that is written to
	// both the outgoing damage modifiers (all of the owner's outgoing damage) and the ship's movement speed modifiers
	// (which scale the speed cap too). Replace runs only when the applier rebuilds, so an unchanged stack count
	// produces no modifier traffic, and a source is removed as soon as its multipliers return to 1.
	class ModuleRunCounterRuntime final
	{
	public:
		explicit ModuleRunCounterRuntime(CombatRuntime& combatRuntime) : mCombatRuntime{ combatRuntime } {}
		ModuleRunCounterRuntime(const ModuleRunCounterRuntime&) = delete;
		ModuleRunCounterRuntime& operator=(const ModuleRunCounterRuntime&) = delete;

		// Movement modifiers belong to the owning ship and must stay valid until Clear runs.
		void Replace(const List<ModulePlacement>& placements, const ModuleLookup& lookup, const ResolvedModuleEffects& effects, ShipRuntimeModifiers* movementModifiers)
		{
			mMovementModifiers = movementModifiers;
			List<Source> desired;
			for (const ModulePlacement& placement : placements)
			{
				const ModuleDefinition* module = lookup ? lookup(placement.moduleId) : nullptr;
				if (!module || placement.variantIndex < 0 || static_cast<std::size_t>(placement.variantIndex) >= module->sizeVariants.size()) continue;
				const List<ModuleRunCounterRule>& rules = module->sizeVariants[static_cast<std::size_t>(placement.variantIndex)].runCounterRules;
				const int level = placement.level + ModuleResolver::FindZoneLevelBonus(effects, placement.instanceId);
				for (std::size_t index = 0; index < rules.size(); ++index)
				{
					const ModuleRunCounterRule& rule = rules[index];
					if (rule.source != ModuleRunCounterSource::Distance) continue;
					const int stacks = GetModuleRunCounterStacks(rule, placement.runCounter);
					if (stacks <= 0) continue;
					const float damage = 1.f + rule.damagePerStackByLevel.ValueAtLevel(rule.damagePerStack, level) * static_cast<float>(stacks);
					const float speed = 1.f + rule.moveSpeedPerStackByLevel.ValueAtLevel(rule.moveSpeedPerStack, level) * static_cast<float>(stacks);
					if (damage == 1.f && speed == 1.f) continue;
					desired.push_back({ "ModuleRunCounter." + std::to_string(placement.instanceId) + "." + std::to_string(index), damage, speed });
				}
			}

			for (const Source& current : mSources)
			{
				const bool kept = std::any_of(desired.begin(), desired.end(), [&current](const Source& next) { return next.id == current.id; });
				if (!kept) Remove(current.id);
			}
			for (const Source& next : desired)
			{
				const auto previous = std::find_if(mSources.begin(), mSources.end(), [&next](const Source& source) { return source.id == next.id; });
				if (previous != mSources.end() && previous->damage == next.damage && previous->speed == next.speed) continue;
				Apply(next);
			}
			mSources = std::move(desired);
		}

		void Clear()
		{
			for (const Source& current : mSources) Remove(current.id);
			mSources.clear();
			for (const KillSource& current : mKillSources) mCombatRuntime.RemoveRuntimeModifier(current.id);
			mKillSources.clear();
			mMovementModifiers = nullptr;
		}

		// Scoped outgoing damage from kill credit (Trophy Core neighbours and Trophy Wall). Each source keeps its id while its
		// value is unchanged; a source absent from sources, or with multiplier 1, leaves the ship at once.
		void ReplaceKillSources(const List<ResolvedModuleKillSource>& sources)
		{
			for (const KillSource& current : mKillSources)
			{
				const bool kept = std::any_of(sources.begin(), sources.end(), [&current](const ResolvedModuleKillSource& next) { return next.id == current.id; });
				if (!kept) mCombatRuntime.RemoveRuntimeModifier(current.id);
			}
			List<KillSource> next;
			next.reserve(sources.size());
			for (const ResolvedModuleKillSource& source : sources)
			{
				if (source.multiplier == 1.f)
				{
					mCombatRuntime.RemoveRuntimeModifier(source.id);
					continue;
				}
				const auto previous = std::find_if(mKillSources.begin(), mKillSources.end(), [&source](const KillSource& owned) { return owned.id == source.id; });
				if (previous != mKillSources.end() && previous->multiplier == source.multiplier && previous->slot == source.slot && previous->family == source.family)
				{
					next.push_back(*previous);
					continue;
				}
				mCombatRuntime.SetRuntimeModifier(source.id, CombatRuntimeModifier{ source.multiplier, source.slot, source.family });
				next.push_back({ source.id, source.slot, source.family, source.multiplier });
			}
			mKillSources = std::move(next);
		}

		std::size_t GetOwnedSourceCount() const { return mSources.size() + mKillSources.size(); }

			// Product of the outgoing damage multipliers this runtime owns (run-counter stacks only); 1 when none.
			float GetDamageMultiplier() const
			{
				float multiplier = 1.f;
				for (const Source& source : mSources) multiplier *= source.damage;
				return multiplier;
			}

	private:
		struct Source
		{
			std::string id;
			float damage = 1.f;
			float speed = 1.f;
		};

		void Apply(const Source& source)
		{
			if (source.damage != 1.f) mCombatRuntime.SetRuntimeModifier(source.id, CombatRuntimeModifier{ source.damage });
			else mCombatRuntime.RemoveRuntimeModifier(source.id);
			if (!mMovementModifiers) return;
			if (source.speed != 1.f)
			{
				ShipRuntimeModifier modifier;
				modifier.movementSpeedMultiplier = source.speed;
				mMovementModifiers->Set(source.id, modifier);
			}
			else mMovementModifiers->Remove(source.id);
		}

		void Remove(const std::string& id)
		{
			mCombatRuntime.RemoveRuntimeModifier(id);
			if (mMovementModifiers) mMovementModifiers->Remove(id);
		}

		struct KillSource
		{
			std::string id;
			sas::AbilitySlot slot = sas::AbilitySlot::None;
			GameplayTag family;
			float multiplier = 1.f;
		};

		CombatRuntime& mCombatRuntime;
		ShipRuntimeModifiers* mMovementModifiers = nullptr;
		List<Source> mSources;
		List<KillSource> mKillSources;
	};
}
