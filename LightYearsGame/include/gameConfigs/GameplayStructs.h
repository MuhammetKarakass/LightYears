#pragma once

#include <SFML/Graphics.hpp>
#include "framework/Core.h"
#include "framework/MathUtility.h"
#include <string>
#include "engineConfigs/EngineStructs.h"
#include "player/Reward.h"

enum class WeaponType
{
	Default,
	ThreeWay,
	FrontalWhiper,
};

struct WeaponState
{
	WeaponType type = WeaponType::Default;
	int level = 1;
};

struct BulletDefinition
{
	std::string texturePath;
	float speed;
	float damage;

	PointLightDefinition pointLightDef;
	sf::Vector2f lightOffset;

	BulletDefinition(
		const std::string& inTexturePath = "SpaceShooterRedux/PNG/Lasers/laserBlue01.png",
		float inSpeed = 500.f,
		float inDamage = 10.f,
		const PointLightDefinition& inPointLightDef = PointLightDefinition(),
		const sf::Vector2f& inOffset = { 0.f, 0.f }
	)
		: texturePath(inTexturePath)
		, speed(inSpeed)
		, damage(inDamage)
		, pointLightDef(inPointLightDef)
		, lightOffset(inOffset)
	{
	}
};

struct EngineMount
{
	sf::Vector2f offset;
	PointLightDefinition pointLightDef;
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

	BulletDefinition bulletDefinition;
	bool hasWeapon;
	sf::Vector2f weaponOffset;
	float weaponCooldown;
	ly::List<ly::WeightedReward> rewards;

	ShipDefinition
	(
		const std::string& inTexturePath,
		float inHealth,
		const sf::Vector2f& inSpeed,
		float inCollisionDamage,
		float inScoreAmt,
		int inExplosionType,
		const ly::List<EngineMount>& inEngineMounts,
		const BulletDefinition& inBulletDefinition,
		bool inHasWeapon,
		const sf::Vector2f& inWeaponOffset,
		float inWeaponCooldown,
		const ly::List<ly::WeightedReward>& inRewards
	)
		: texturePath(inTexturePath)
		, health(inHealth)
		, speed(inSpeed)
		, collisionDamage(inCollisionDamage)
		, scoreAmt(inScoreAmt)
		, explosionType(inExplosionType)
		, engineMounts(inEngineMounts)
		, bulletDefinition(inBulletDefinition)
		, hasWeapon(inHasWeapon)
		, weaponOffset(inWeaponOffset)
		, weaponCooldown(inWeaponCooldown)
		, rewards(inRewards)
	{
	}
};

