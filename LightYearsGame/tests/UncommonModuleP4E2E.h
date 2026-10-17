#pragma once

#include "gameConfigs/CollisionLayers.h"
#include "CommonModuleE2ETestSupport.h"
#include "gameplay/content/ModuleSideFeatureCatalog.h"
#include "gameplay/content/ModuleLoader.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/module/ModuleAbilitySlots.h"

namespace uncommon_p4_e2e
{
	using namespace ly;
	using namespace common_module_e2e;
	struct Fixture
	{
		Report& report;
		shared_ptr<HeadlessModuleLevel> level;
		shared_ptr<PlayerSpaceShip> ship;
		ModuleRulesContext rules;
		ModuleLoadout loadout;
		Fixture(Report& result, Application& application) : report{ result }, level{ std::make_shared<HeadlessModuleLevel>(&application) }, rules{ *content::ShipContentCatalog::GetPlayerFighterDefinition().schematic, [](const sas::ContentId& id) { return content::ModuleContentCatalog::FindById(id); }, content::ModuleContentCatalog::GetZoneTable(), [](const sas::ContentId& id) { return content::ModuleSideFeatureCatalog::FindById(id); } }
		{
			level->BeginPlayInternal();
			const auto& definition = content::ShipContentCatalog::GetPlayerShipDefinition("Ship.Player.Fighter.Basic");
			ship = level->SpawnActor<PlayerSpaceShip>(definition).lock();
			report.Check(ship != nullptr, "P4 spawns a real player ship");
			level->TickInternal(0.f);
			ship->SetInvulnerability(false);
			ship->SetUseScreenClamp(false);
			ship->SetActorLocation({ 10000.f, 10000.f });
			report.Check(loadout.SeedDefaults(rules) && ship->GetModuleApplier().Apply(loadout, rules), "P4 uses canonical default modules and runtime applier");
		}
		std::uint32_t Install(const char* id, int variant = 0, int levelValue = 1, const char* side = "")
		{
			const auto* definition = rules.lookup(id);
			report.Check(definition != nullptr, std::string{ "P4 shipped module: " } + id);
			const auto instance = loadout.AddToInventory(rules, *definition, variant, definition->rarity, levelValue, sas::ContentId{ side });
			report.Check(instance != 0, "P4 inventory validates variant, level and feature");
			ModulePlacement candidate = *loadout.FindInventoryItem(instance);
			for (int rotation = 0; rotation < 4; ++rotation)
				for (int y = 0; y < rules.schematic.height; ++y)
					for (int x = 0; x < rules.schematic.width; ++x)
					{
						candidate.origin = { x, y };
						candidate.rotationQuarterTurns = rotation;
						if (ModulePlacementRules::CanPlace(rules, loadout, candidate) != ModulePlacementFailure::None) continue;
						report.Check(loadout.Install(rules, instance, candidate.origin, rotation) == ModulePlacementFailure::None && ship->GetModuleApplier().Apply(loadout, rules), "P4 installs through real placement and applier");
						return instance;
					}
			throw std::runtime_error{ "No legal P4 placement" };
		}
		void Remove(std::uint32_t id)
		{
			report.Check(loadout.Uninstall(id) && ship->GetModuleApplier().Apply(loadout, rules), "P4 removes module through the real applier");
		}
		shared_ptr<SpaceShip> Kill(Actor* source = nullptr, bool generated = false)
		{
			const auto& definition = content::ShipContentCatalog::GetPlayerShipDefinition("Ship.Player.Fighter.Basic");
			const auto enemy = level->SpawnActor<SpaceShip>(definition).lock();
			report.Check(enemy != nullptr, "P4 creates a real enemy combatant");
			level->TickInternal(0.f);
			enemy->SetCollisionLayer(ly::GameCollision::Enemy);
			enemy->SetActorLocation({ 20000.f, 20000.f });
			DamagePayload payload;
			payload.criticalPolicy = DamageCriticalPolicy::Disabled;
			payload.kineticStacks = 0;
			payload.skipModuleDamageRules = generated;
			ApplyCombatDamage(*enemy, 100000.f, source ? source : ship.get(), {}, payload);
			report.Check(enemy->GetHealthComponent().GetHealth() == 0.f, "P4 confirms kill through actual damage and health resolution");
			return enemy;
		}
		float Owner(const sas::AttributeId& id) const { return ship->GetAbilitySystemComponent().GetAttributes().GetCurrentValue(id); }
		void Equal(const char* name, float expected, float actual) { report.CheckValue(name, expected, actual, "P4 live ship, canonical loadout and content"); }
	};
	inline shared_ptr<SpaceShip> Enemy(Fixture& f, float distance = 50.f)
	{
		const auto enemy = f.level->SpawnActor<SpaceShip>(content::ShipContentCatalog::GetPlayerFighterDefinition()).lock();
		f.level->TickInternal(0.f);
		enemy->SetCollisionLayer(ly::GameCollision::Enemy);
		enemy->SetCollisionMask(CollisionLayer::None);
		enemy->SetEnablePhysics(false);
		enemy->SetActorLocation(f.ship->GetActorLocation() + sf::Vector2f{ distance, 0.f });
		enemy->GetShieldComponent().ChangeShield(-10000.f);
		enemy->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Armor, 0.f);
		enemy->GetHealthComponent().SetMaxHealth(1000.f);
		enemy->GetHealthComponent().Regenerate(1000.f);
		return enemy;
	}
	inline float Hit(Actor& target, Actor* source, float damage = 10.f, DamagePayload payload = {}, sas::ContentId ability = {}, List<GameplayTag> abilityTags = {}, DamageDeliveryType delivery = DamageDeliveryType::Direct, List<GameplayTag> damageTags = {})
	{
		const auto* health = target.FindComponent<HealthComponent>();
		const float before = health->GetHealth();
		ApplyCombatDamage(target, damage, source, damageTags, payload, ability, abilityTags, delivery);
		return before - health->GetHealth();
	}
	inline DamagePayload Plain()
	{
		DamagePayload payload;
		payload.criticalPolicy = DamageCriticalPolicy::Disabled;
		payload.kineticStacks = 0;
		return payload;
	}
	void DamageChecks(Report& report, Application& application);
	void ReactionChecks(Report& report, Application& application);
	void BridgeChecks(Report& report, Application& application);
}
