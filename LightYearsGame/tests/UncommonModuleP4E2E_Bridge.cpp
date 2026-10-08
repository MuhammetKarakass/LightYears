#include "UncommonModuleP4E2E.h"

namespace uncommon_p4_e2e
{
	namespace
	{
		struct HitTrace
		{
			CombatRuntime& runtime;
			DelegateHandle handle;
			List<GameplayTag> tags;
			int count = 0;
			explicit HitTrace(CombatRuntime& value) : runtime{ value }, handle{ runtime.onDamageResolved.BindAction(this, &HitTrace::OnHit) } {}
			~HitTrace() { runtime.onDamageResolved.UnbindAction(handle); }
			void OnHit(const DamageContext& context) { tags = context.damageTags; ++count; }
		};
	}
	void BridgeChecks(Report& report, Application& application)
	{
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application };
			const auto target = Enemy(f);
			HitTrace trace{ target->GetCombatRuntime() };
			const auto bridge = f.Install("Module.Defense.HealthBridge.Uncommon", 0, level);
			ModulePlacement geometryBridge = *f.loadout.FindPlacement(bridge);
			geometryBridge.origin = { 4, 2 }; geometryBridge.rotationQuarterTurns = 0;
			List<ModulePlacement> geometry{ geometryBridge };
			for (const auto& placement : f.loadout.GetPlacements())
			{
				const auto* definition = f.rules.lookup(placement.moduleId);
				ModulePlacement neighbor = placement;
				if (definition->systemCategory == GameplayTags::Module::System::AbilitySlot1) neighbor.origin = { 4, 1 };
				else if (definition->systemCategory == GameplayTags::Module::System::AbilitySlot2) neighbor.origin = { 4, 3 };
				else if (definition->systemCategory == GameplayTags::Module::System::AbilitySlot3) neighbor.origin = { 2, 2 };
				else if (definition->systemCategory == GameplayTags::Module::System::PrimaryWeapon) neighbor.origin = { 6, 2 };
				else continue;
				geometry.push_back(neighbor);
			}
			report.Check(ModulePlacementRules::GetAdjacentAbilitySlotSystems(f.rules, geometry, geometryBridge, 4).size() == 3 && ModulePlacementRules::GetAdjacentAbilitySlotSystems(f.rules, geometry, geometryBridge).size() == 2, "Bridge ignores Primary and caps three geometric slot neighbors at two");
			ModulePlacement diagonal = geometry[1]; diagonal.origin = { 6, 3 };
			report.Check(!ModulePlacementRules::SharesCellEdge(*f.rules.lookup(geometryBridge.moduleId), geometryBridge, *f.rules.lookup(diagonal.moduleId), diagonal), "diagonal contact is not adjacency");
			for (std::size_t count : { 0u, 1u, 2u })
			{
				bool found = false;
				ModulePlacement candidate = *f.loadout.FindPlacement(bridge);
				for (int rotation = 0; rotation < 4 && !found; ++rotation)
					for (int y = 0; y < f.rules.schematic.height && !found; ++y)
						for (int x = 0; x < f.rules.schematic.width && !found; ++x)
						{
							candidate.origin = { x, y }; candidate.rotationQuarterTurns = rotation;
							if (ModulePlacementRules::CanPlace(f.rules, f.loadout, candidate, bridge) != ModulePlacementFailure::None) continue;
							const auto systems = ModulePlacementRules::GetAdjacentAbilitySlotSystems(f.rules, f.loadout.GetPlacements(), candidate);
							if (systems.size() != count) continue;
							report.Check(f.loadout.Move(f.rules, bridge, candidate.origin, rotation) == ModulePlacementFailure::None, "Bridge moves through canonical placement rules");
							f.ship->GetModuleApplier().Apply(f.loadout, f.rules);
							found = true;
						}
				report.Check(found, "real Fighter grid supports zero, one and two neighbors", count);
				const auto& resolved = f.ship->GetModuleApplier().GetResolvedEffects().damageRules;
				const auto rule = std::find_if(resolved.begin(), resolved.end(), [bridge](const auto& value) { return value.instanceId == bridge; });
				report.Check(rule != resolved.end() && rule->adjacentAbilitySystems.size() == count, "resolver captures only active adjacent ability systems");
				for (auto slot : { sas::AbilitySlot::Ability1, sas::AbilitySlot::Ability2, sas::AbilitySlot::Ability3, sas::AbilitySlot::Ability4 })
				{
					const auto* ability = f.ship->GetAbilitySystemComponent().GetAbility(slot);
					const bool adjacent = std::any_of(rule->adjacentAbilitySystems.begin(), rule->adjacentAbilitySystems.end(), [slot](const auto& system) { return AbilitySlotForModuleSystem(system) == slot; });
					const float bonus = adjacent ? f.ship->GetHealthComponent().GetMaxHealth() * (.03f + .005f * (level - 1)) : 0.f;
					const int countBeforeHit = trace.count;
					f.Equal("Bridge adds MaxHealth damage only to adjacent slot hits", 10.f + bonus, Hit(*target, f.ship.get(), 10.f, Plain(), ability->GetDefinition().abilityId, ability->GetDefinition().abilityTags, DamageDeliveryType::Direct, { DamageTypeSchema::Cryo }));
					report.Check(trace.count == countBeforeHit + 1 && trace.tags == List<GameplayTag>{ DamageTypeSchema::Cryo }, "Bridge preserves the ability damage type in one direct hit");
					DamagePayload critical = Plain(); critical.criticalPolicy = DamageCriticalPolicy::Guaranteed;
					f.Equal("Bridge adds damage before critical multiplication", (10.f + bonus) * f.ship->GetCombatRuntime().GetCriticalDamageMultiplier(), Hit(*target, f.ship.get(), 10.f, critical, ability->GetDefinition().abilityId, ability->GetDefinition().abilityTags));
					f.Equal("Bridge excludes DoT ticks from adjacent slots", 10.f, Hit(*target, f.ship.get(), 10.f, Plain(), ability->GetDefinition().abilityId, ability->GetDefinition().abilityTags, DamageDeliveryType::Periodic));
				}
				f.Equal("Bridge excludes Primary damage", 10.f, Hit(*target, f.ship.get(), 10.f, Plain(), {}, { GameplayTags::Ability::Primary }));
			}
			ModulePlacement rotated = *f.loadout.FindPlacement(bridge);
			rotated.rotationQuarterTurns = 1;
			bool rotatedFound = false;
			for (int y = 0; y < f.rules.schematic.height && !rotatedFound; ++y)
				for (int x = 0; x < f.rules.schematic.width && !rotatedFound; ++x)
				{
					rotated.origin = { x, y };
					if (ModulePlacementRules::CanPlace(f.rules, f.loadout, rotated, bridge) != ModulePlacementFailure::None) continue;
					const auto adjacent = ModulePlacementRules::GetAdjacentAbilitySlotSystems(f.rules, f.loadout.GetPlacements(), rotated);
					if (adjacent.empty()) continue;
					report.Check(f.loadout.Move(f.rules, bridge, rotated.origin, 1) == ModulePlacementFailure::None && f.ship->GetModuleApplier().Apply(f.loadout, f.rules), "rotated Bridge applies through live placement and resolver");
					const auto* selected = f.ship->GetAbilitySystemComponent().GetAbility(AbilitySlotForModuleSystem(adjacent.front()));
					f.Equal("rotated Bridge uses edge-adjacent cells", 10.f + f.ship->GetHealthComponent().GetMaxHealth() * (.03f + .005f * (level - 1)), Hit(*target, f.ship.get(), 10.f, Plain(), selected->GetDefinition().abilityId, selected->GetDefinition().abilityTags));
					rotatedFound = true;
				}
			report.Check(rotatedFound, "Fighter has a legal rotated Bridge with a neighbor");
			f.Remove(bridge);
			const auto* ability = f.ship->GetAbilitySystemComponent().GetAbility(sas::AbilitySlot::Ability1);
			f.Equal("Bridge removes its resolved bonus", 10.f, Hit(*target, f.ship.get(), 10.f, Plain(), ability->GetDefinition().abilityId, ability->GetDefinition().abilityTags));
		}
		Fixture f{ report, application };
		const auto target = Enemy(f);
		const auto baseline = f.ship->GetModuleApplier().GetOwnedHandleCount();
		for (int cycle = 0; cycle < 10; ++cycle)
		{
			const auto rhythm = f.Install("Module.Offense.SharpScale.Uncommon", 1, 3, "Module.SideFeature.SharpScale.CriticalRhythm");
			DamagePayload critical = Plain(); critical.criticalPolicy = DamageCriticalPolicy::Guaranteed;
			Hit(*target, f.ship.get(), 1.f, critical);
			f.Remove(rhythm);
			report.Check(f.ship->GetModuleTriggerRuntime().GetActiveModifierHandleCount() == 0 && f.ship->GetModuleApplier().GetOwnedHandleCount() == baseline, "ten critical install/remove cycles leak no handles");
			const auto luck = f.Install("Module.Offense.SharpScale.Uncommon", 1, 3, "Module.SideFeature.SharpScale.LuckLens");
			f.ship->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Luck, 5.f);
			f.Remove(luck);
			report.Check(f.ship->GetModuleApplier().GetOwnedHandleCount() == baseline, "ten dynamic Luck install/remove cycles leak no handles");
		}
		const auto rejects = [&report](const Json& json)
		{
			bool rejected = false;
			try { content::ModuleLoader::ParseRulePackage(json, "P4 invalid content", 3, false); }
			catch (const std::exception&) { rejected = true; }
			report.Check(rejected, "P4 loader rejects malformed numeric/type contracts", json);
		};
		rejects({ { "shieldReflections", Json::array({ { { "fraction", { .35f, .4f } }, { "durationSeconds", 5.f } } }) } });
		rejects({ { "shieldReflections", Json::array({ { { "fraction", .35f }, { "durationSeconds", 0.f } } }) } });
		rejects({ { "damageRules", Json::array({ { { "condition", "TargetWithoutStatus" }, { "maxHealthDamageFraction", .03f } } }) } });
		rejects({ { "eventActions", Json::array({ { { "trigger", "DamageTaken" }, { "kind", "ApplyNearbyCryo" }, { "amount", .5f }, { "radius", 300.f }, { "cooldownSeconds", 2.f }, { "targetCount", { 2, 3, 4 } } } }) } });
		rejects({ { "eventActions", Json::array({ { { "trigger", "DamageTaken" }, { "kind", "ApplyNearbyCryo" }, { "amount", 1.f }, { "radius", 300.f }, { "cooldownSeconds", 2.f }, { "targetCount", { 2, 3 } } } }) } });
	}
}
