#pragma once
// Fragment, not a standalone header: ModuleDefinition.h includes it inside namespace vega after ModuleLevelTable and before
// ModuleRulePackage, so the rule structs can use ModuleLevelTable. Keep it free of namespaces and includes.

	// Shield Battery: a reserve kept beyond the normal shield. The main entry's reserveFractionByLevel caps the reserve at that
	// fraction of Max Shield and it fills only while the normal shield is full; incoming damage spends it before the normal shield.
	// The side entry (Unbroken Storage) sets damagedFillFraction: the share of the normal regeneration rate that keeps filling the
	// reserve during the post-damage recharge delay while the normal shield is full. 0 = off.
	struct ModuleShieldBatteryRule
	{
		float reserveFraction = 0.f;
		ModuleLevelTable<float> reserveFractionByLevel;
		float damagedFillFraction = 0.f;
	};
	inline bool operator==(const ModuleShieldBatteryRule& left, const ModuleShieldBatteryRule& right) { return left.reserveFraction == right.reserveFraction && left.reserveFractionByLevel.values == right.reserveFractionByLevel.values && left.damagedFillFraction == right.damagedFillFraction; }

	// Shield Rupture: when an owner hit breaks an enemy's shield, an Energy explosion at that enemy deals damageFraction of the shield
	// the owner removed from it within the last historySeconds, over radius. Module explosions never trigger it.
	struct ModuleShieldRuptureRule
	{
		float damageFraction = 0.5f;
		ModuleLevelTable<float> damageFractionByLevel;
		float radius = 150.f;
		float historySeconds = 3.f;
	};
	inline bool operator==(const ModuleShieldRuptureRule& left, const ModuleShieldRuptureRule& right) { return left.damageFraction == right.damageFraction && left.damageFractionByLevel.values == right.damageFractionByLevel.values && left.radius == right.radius && left.historySeconds == right.historySeconds; }

	struct ResolvedModuleShieldBatteryRule { std::uint32_t instanceId = 0; std::size_t ruleIndex = 0; ModuleShieldBatteryRule rule; };
	struct ResolvedModuleShieldRuptureRule { std::uint32_t instanceId = 0; std::size_t ruleIndex = 0; ModuleShieldRuptureRule rule; };
