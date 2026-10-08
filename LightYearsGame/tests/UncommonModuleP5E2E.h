#pragma once
#include "UncommonModuleP4E2E.h"

namespace uncommon_p5_e2e
{
	// Reuse P4's real ship/loadout fixture and damage helpers.
	using namespace uncommon_p4_e2e;
	inline const sas::ActiveGameplayEffect* Effect(const SpaceShip& ship, const char* id) { return ship.GetAbilitySystemComponent().FindGameplayEffectById(id); }
	inline int Stacks(const SpaceShip& ship, const char* id) { const auto* effect = Effect(ship, id); return effect ? effect->stackCount : 0; }
	inline void StatusHit(SpaceShip& target, Actor* source, const GameplayTag& type, int stacks = 1, bool suppressed = false)
	{
		auto payload = Plain();
		if (type == DamageTypeSchema::Electric) payload.electricStacks = stacks;
		if (type == DamageTypeSchema::Cryo) payload.cryoBuildupPerHit = stacks;
		if (type == DamageTypeSchema::Thermal) payload.igniteStacks = stacks;
		if (suppressed) payload.moduleTriggerPolicy = DamageModuleTriggerPolicy::Suppressed;
		Hit(target, source, 0.1f, payload, {}, {}, DamageDeliveryType::Direct, { type });
	}
	void StatusChecks(Report& report, Application& application);
	void ContentChecks(Report& report, const std::filesystem::path& artifactRoot);
}
