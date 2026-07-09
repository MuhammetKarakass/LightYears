#pragma once

#include <SFML/Graphics.hpp>
#include "framework/Core.h"
#include "framework/MathUtility.h"
#include "gameplay/GameplayAttribute.h"
#include <string>
#include "engineConfigs/EngineStructs.h"
#include "player/Reward.h"

enum class PrimaryWeaponDeliveryType
{
	Projectile,
	Beam,
	AreaPulse,
	Orbital,
	Chain
};

enum class PrimaryWeaponFirePattern
{
	Single,
	Spread,
	Ring
};

struct WeaponPresentationDefinition
{
	std::string texturePath;
	PointLightDefinition pointLightDef;
	sf::Vector2f lightOffset;
	float visualScale;

	WeaponPresentationDefinition(
		const std::string& inTexturePath = "SpaceShooterRedux/PNG/Lasers/laserBlue01.png",
		const PointLightDefinition& inPointLightDef = PointLightDefinition(),
		const sf::Vector2f& inLightOffset = { 0.f, 0.f },
		float inVisualScale = 1.f
	)
		: texturePath(inTexturePath)
		, pointLightDef(inPointLightDef)
		, lightOffset(inLightOffset)
		, visualScale(inVisualScale)
	{
	}
};

struct PrimaryWeaponAttributes
{
	ly::GameplayAttribute damage;
	ly::GameplayAttribute shotsPerSecond;
	ly::GameplayAttribute projectileSpeed;
	ly::GameplayAttribute projectileLifeTime;
	ly::GameplayAttribute projectileMaxTravelDistance;
	ly::GameplayAttribute projectileCollisionRadius;
	ly::GameplayAttribute projectileAreaRadius;
	ly::GameplayAttribute spreadAngle;
	ly::GameplayAttribute projectilesPerShot;
	ly::GameplayAttribute pierceCount;
	ly::GameplayAttribute beamRange;
	ly::GameplayAttribute beamWidth;

	PrimaryWeaponAttributes(
		float inDamage = 10.f,
		float inShotsPerSecond = 5.f,
		float inProjectileSpeed = 500.f,
		float inProjectileLifeTime = 3.f,
		float inProjectileMaxTravelDistance = 1600.f,
		float inProjectileCollisionRadius = 8.f,
		float inProjectileAreaRadius = 0.f,
		float inSpreadAngle = 0.f,
		float inProjectilesPerShot = 1.f,
		float inPierceCount = 0.f,
		float inBeamRange = 0.f,
		float inBeamWidth = 0.f
	)
		: damage{ inDamage },
		shotsPerSecond{ inShotsPerSecond },
		projectileSpeed{ inProjectileSpeed },
		projectileLifeTime{ inProjectileLifeTime },
		projectileMaxTravelDistance{ inProjectileMaxTravelDistance },
		projectileCollisionRadius{ inProjectileCollisionRadius },
		projectileAreaRadius{ inProjectileAreaRadius },
		spreadAngle{ inSpreadAngle },
		projectilesPerShot{ inProjectilesPerShot },
		pierceCount{ inPierceCount },
		beamRange{ inBeamRange },
		beamWidth{ inBeamWidth }
	{
	}
};

struct WeaponMuzzleDefinition
{
	sf::Vector2f offset;
	float rotationOffset;

	WeaponMuzzleDefinition(
		const sf::Vector2f& inOffset = { 0.f, 50.f },
		float inRotationOffset = 0.f
	)
		: offset(inOffset)
		, rotationOffset(inRotationOffset)
	{
	}
};

struct PrimaryWeaponDefinition
{
	std::string weaponId;
	PrimaryWeaponDeliveryType deliveryType;
	WeaponPresentationDefinition presentationDefinition;
	PrimaryWeaponAttributes attributes;
	ly::List<WeaponMuzzleDefinition> muzzleDefinitions;
	PrimaryWeaponFirePattern firePattern;
	bool automaticFire;

	PrimaryWeaponDefinition(
		const std::string& inWeaponId = "DefaultPrimaryWeapon",
		PrimaryWeaponDeliveryType inDeliveryType = PrimaryWeaponDeliveryType::Projectile,
		const WeaponPresentationDefinition& inPresentationDefinition = WeaponPresentationDefinition{},
		const PrimaryWeaponAttributes& inAttributes = PrimaryWeaponAttributes{},
		const ly::List<WeaponMuzzleDefinition>& inMuzzleDefinitions = { WeaponMuzzleDefinition{} },
		PrimaryWeaponFirePattern inFirePattern = PrimaryWeaponFirePattern::Single,
		bool inAutomaticFire = true
	)
		: weaponId(inWeaponId)
		, deliveryType(inDeliveryType)
		, presentationDefinition(inPresentationDefinition)
		, attributes(inAttributes)
		, muzzleDefinitions(inMuzzleDefinitions)
		, firePattern(inFirePattern)
		, automaticFire(inAutomaticFire)
	{
	}
};

struct EngineMount
{
	sf::Vector2f offset;
	PointLightDefinition pointLightDef;
};

struct ShipMovementAttributes
{
	ly::GameplayAttribute forwardThrust;
	ly::GameplayAttribute reverseThrust;
	ly::GameplayAttribute strafeThrust;
	ly::GameplayAttribute angularTurnSpeed;
	ly::GameplayAttribute angularTurnResponsiveness;
	ly::GameplayAttribute linearDamping;
	ly::GameplayAttribute maxSpeed;
	ly::GameplayAttribute inputResponsiveness;
	ly::GameplayAttribute mouseAimDeadZone;

	ShipMovementAttributes(
		float inForwardThrust = 350.f,
		float inReverseThrust = 120.f,
		float inStrafeThrust = 150.f,
		float inAngularTurnSpeed = 100.f,
		float inAngularTurnResponsiveness = 2.f,
		float inLinearDamping = 0.55f,
		float inMaxSpeed = 520.f,
		float inInputResponsiveness = 14.f,
		float inMouseAimDeadZone = 28.f
	)
		: forwardThrust{ inForwardThrust },
		reverseThrust{ inReverseThrust },
		strafeThrust{ inStrafeThrust },
		angularTurnSpeed{ inAngularTurnSpeed },
		angularTurnResponsiveness{ inAngularTurnResponsiveness },
		linearDamping{ inLinearDamping },
		maxSpeed{ inMaxSpeed },
		inputResponsiveness{ inInputResponsiveness },
		mouseAimDeadZone{ inMouseAimDeadZone }
	{
	}
};

struct ShipDefinition
{
	std::string texturePath;
	float health;
	sf::Vector2f speed;

	float collisionDamage;
	unsigned int scoreAmt;
	int explosionType;
	
	ly::List<EngineMount> engineMounts;

	ly::List<ly::WeightedReward> rewards;
	PrimaryWeaponDefinition primaryWeaponDefinition;
	ShipMovementAttributes movementAttributes;

	ShipDefinition
	(
		const std::string& inTexturePath,
		float inHealth,
		const sf::Vector2f& inSpeed,
		float inCollisionDamage,
		float inScoreAmt,
		int inExplosionType,
		const ly::List<EngineMount>& inEngineMounts,
		const ly::List<ly::WeightedReward>& inRewards,
		const PrimaryWeaponDefinition& inPrimaryWeaponDefinition = PrimaryWeaponDefinition{},
		const ShipMovementAttributes& inMovementAttributes = ShipMovementAttributes{}
	)
		: texturePath(inTexturePath)
		, health(inHealth)
		, speed(inSpeed)
		, collisionDamage(inCollisionDamage)
		, scoreAmt(inScoreAmt)
		, explosionType(inExplosionType)
		, engineMounts(inEngineMounts)
		, rewards(inRewards)
		, primaryWeaponDefinition(inPrimaryWeaponDefinition)
		, movementAttributes(inMovementAttributes)
	{
	}
};

