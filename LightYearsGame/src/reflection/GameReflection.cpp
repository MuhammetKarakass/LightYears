#include "reflection/GameReflection.h"
#include <framework/Actor.h>
#include <framework/reflection/Reflection.h>
#include "gameplay/HealthComponent.h"
#include "gameplay/ShieldComponent.h"
#include "gameplay/EnergyComponent.h"
#include "gameplay/MovementComponent.h"
#include "player/PlayerMovementComponent.h"
#include "player/PlayerSpaceShip.h"
#include "enemy/EnemyActor.h"
#include "enemy/DummyEnemy.h"
#include "environment/Asteroid.h"
#include "level/PlayerStart.h"
#include "level/SpriteActor.h"
#include "gameplay/content/ShipContentCatalog.h"
#include <framework/scene/Scene.h>
#include <framework/scene/SceneEntityComponent.h>

namespace ly
{
	void RegisterGameTypes()
	{
		reflect::RegisterEngineTypes();
		static bool registered = false;
		if (registered) return;
		registered = true;
		using reflect::Class;
		Class<Asteroid, Actor>("Asteroid").Spawnable()
			.Property("Velocity", &Asteroid::GetAsteroidVelocity, &Asteroid::SetAsteroidVelocity).Category("Asteroid");
		Class<PlayerStart, Actor>("PlayerStart").Spawnable()
			.Property("Ship", &PlayerStart::GetShip, &PlayerStart::SetShip).Category("Player Start");
		Class<SpriteActor, Actor>("SpriteActor").Spawnable();
		Class<DummyEnemy, Actor>("DummyEnemy")
			.Spawnable([](World& world)
			{
				const auto* ship = content::ShipContentCatalog::FindById("Ship.Enemy.RangeKeeper.Basic");
				if (!ship) throw std::runtime_error("Default dummy ship is unavailable");
				return world.SpawnActor<DummyEnemy>(*ship).lock();
			})
			.Property("Ship", [](const DummyEnemy& actor)
			{
				const auto* identity = actor.FindComponent<SceneEntityComponent>();
				return identity ? identity->spawnData.value("Ship", std::string{ "Ship.Enemy.RangeKeeper.Basic" })
					: std::string{ "Ship.Enemy.RangeKeeper.Basic" };
			}, [](DummyEnemy& actor, const std::string& ship)
			{
				if (!content::ShipContentCatalog::FindById(ship)) throw std::runtime_error("Unknown dummy ship: " + ship);
				actor.GetOrAddComponent<SceneEntityComponent>().spawnData["Ship"] = ship;
			}).Category("Dummy").Tooltip("Construction only: reload the scene to apply a changed ship.");
		SceneSerializer::RegisterSpawnFactory("DummyEnemy", [](World& world, const nlohmann::json& properties)
		{
			const auto id = properties.value("Ship", std::string{ "Ship.Enemy.RangeKeeper.Basic" });
			const auto* ship = content::ShipContentCatalog::FindById(id);
			if (!ship) throw std::runtime_error("Unknown dummy ship: " + id);
			return world.SpawnActor<DummyEnemy>(*ship).lock();
		});
		// Default arguments are not part of a member-function pointer. These
		// adapters retain the existing default resource adjustment semantics.
		Class<HealthComponent, ActorComponent>("HealthComponent")
			.Property("Health", &HealthComponent::GetHealth).Category("Health")
			.Property("Max Health", &HealthComponent::GetMaxHealth, [](HealthComponent& component, float value) { component.SetMaxHealth(value); })
			.Range(1.f, 100000.f).Category("Health");
		Class<ShieldComponent, ActorComponent>("ShieldComponent")
			.Property("Shield", &ShieldComponent::GetShield).Category("Shield")
			.Property("Max Shield", &ShieldComponent::GetMaxShield, [](ShieldComponent& component, float value) { component.SetMaxShield(value); }).Range(0.f, 100000.f).Category("Shield")
			.Property("Recharge Delay", &ShieldComponent::GetRechargeDelay, &ShieldComponent::SetRechargeDelay).Range(0.f, 1000.f).Speed(0.1f).Category("Shield")
			.Property("Recharge Delay Remaining", &ShieldComponent::GetRechargeDelayRemaining).Category("Shield")
			.Property("Module Temporary Shield", &ShieldComponent::GetModuleTemporaryShield).Category("Shield")
			.Property("Front Shield", &ShieldComponent::GetFrontShield).Category("Shield")
			.Property("Rear Shield", &ShieldComponent::GetRearShield).Category("Shield")
			.Property("Passive Regen Blocked", &ShieldComponent::IsPassiveRegenBlocked).Category("Shield");
		Class<EnergyComponent, ActorComponent>("EnergyComponent")
			.Property("Energy", &EnergyComponent::GetEnergy).Category("Energy")
			.Property("Max Energy", &EnergyComponent::GetMaxEnergy, [](EnergyComponent& component, float value) { component.SetMaxEnergy(value); }).Range(0.f, 100000.f).Category("Energy")
			.Property("Recharge Delay", &EnergyComponent::GetRechargeDelay, &EnergyComponent::SetRechargeDelay).Range(0.f, 1000.f).Speed(0.1f).Category("Energy")
			.Property("Recharge Delay Remaining", &EnergyComponent::GetRechargeDelayRemaining).Category("Energy");
		Class<MovementComponent, ActorComponent>("MovementComponent")
			.Property("Movement Mode", &MovementComponent::GetMovementMode, &MovementComponent::SetMovementMode).Labels({ { 0, "LegacyVelocity" }, { 1, "ThrustDrift" } }).Category("Movement")
			.Property("Legacy Speed", &MovementComponent::GetLegacySpeed, &MovementComponent::SetLegacySpeed).Category("Movement")
			.Property("Current Movement Speed", &MovementComponent::GetCurrentMovementSpeed).Category("Movement")
			.Property("Current Movement Distance", &MovementComponent::GetCurrentMovementDistance).Category("Movement")
			.Property("Current Movement Start Location", &MovementComponent::GetCurrentMovementStartLocation).Category("Movement")
			.Property("Directional Thrust Sync", &MovementComponent::HasDirectionalThrustSync).Category("Movement")
			.Property("Module Directional Thrust Sync", &MovementComponent::HasModuleDirectionalThrustSync).Category("Movement")
			.Property("Module Thrust Bonus", &MovementComponent::GetModuleThrustBonus).Category("Movement")
			.Property("Movement Burst Active", &MovementComponent::IsMovementBurstActive).Category("Movement")
			.Property("Resolved Movement Burst Distance", &MovementComponent::GetResolvedMovementBurstDistance).Category("Movement");
		Class<PlayerMovementComponent, ActorComponent>("PlayerMovementComponent")
			.Property("Speed", &PlayerMovementComponent::GetSpeed, &PlayerMovementComponent::SetSpeed).Range(0.f, 100000.f).Category("Player Movement")
			.Property("Use Screen Clamp", &PlayerMovementComponent::GetUseScreenClamp, &PlayerMovementComponent::SetUseScreenClamp).Category("Player Movement")
			.Property("Afterburning", &PlayerMovementComponent::IsAfterburning).Category("Player Movement")
			.Property("Afterburner Camera Zoom Out", &PlayerMovementComponent::GetAfterburnerCameraZoomOut).Category("Player Movement")
			.Property("Speed Cap Multiplier", &PlayerMovementComponent::GetMovementSpeedCapMultiplier).Category("Player Movement")
			.Property("Turn Capability Multiplier", &PlayerMovementComponent::GetMovementTurnCapabilityMultiplier).Category("Player Movement")
			.Property("Afterburner Recharge Blocked", &PlayerMovementComponent::IsAfterburnerRechargeBlocked).Category("Player Movement");
		Class<PlayerSpaceShip, Actor>("PlayerSpaceShip")
			.Property("Speed", &PlayerSpaceShip::GetSpeed, &PlayerSpaceShip::SetSpeed).Range(0.f, 100000.f).Category("Player")
			.Property("Use Screen Clamp", &PlayerSpaceShip::GetUseScreenClamp, &PlayerSpaceShip::SetUseScreenClamp).Category("Player")
			.Property("In Combat", &PlayerSpaceShip::IsInCombat).Category("Player")
			.Property("Combat Window Remaining", &PlayerSpaceShip::GetCombatWindowRemaining).Category("Player");
		Class<EnemyActor, Actor>("EnemyActor")
			.Property("Enemy ID", &EnemyActor::GetEnemyId).Category("Enemy")
			.Property("Combat Profile ID", &EnemyActor::GetCombatProfileId).Category("Enemy")
			.Property("Enemy Level", &EnemyActor::GetEnemyLevel).Category("Enemy")
			.Property("Ship XP Reward", &EnemyActor::GetShipXPReward, &EnemyActor::SetShipXPReward).Range(0.f, 100000.f).Category("Enemy")
			.Property("Collision Damage", &EnemyActor::GetCollisionDamage).Category("Enemy");
	}
}
