#pragma once

#include <SFML/Graphics.hpp>
#include "framework/Core.h"
#include "framework/MathUtility.h"
#include "gameplay/attributes/AttributeSystem.h"
#include "engineConfigs/EngineStructs.h"
#include <string>

struct PrimaryWeaponSchema
{
	// A definition selects exactly one concrete leaf type, never a family tag.
	struct Projectile
	{
		inline static const ly::GameplayTag FamilyId{ "PrimaryWeapon.Projectile" };

		struct Delivery
		{
			inline static const ly::GameplayTag AttributeRoot{ "Attribute.PrimaryWeapon.Projectile.Delivery" };
			inline static const ly::GameplayTag Speed{ "Attribute.PrimaryWeapon.Projectile.Delivery.Speed" };
			inline static const ly::GameplayTag Lifetime{ "Attribute.PrimaryWeapon.Projectile.Delivery.Lifetime" };
			inline static const ly::GameplayTag PierceCount{ "Attribute.PrimaryWeapon.Projectile.Delivery.PierceCount" };
		};

		struct Standard
		{
			inline static const ly::GameplayTag TypeId{ "PrimaryWeapon.Projectile.Standard" };
		};

		struct Shotgun
		{
			inline static const ly::GameplayTag TypeId{ "PrimaryWeapon.Projectile.Shotgun" };
			inline static const ly::GameplayTag AttributeRoot{ "Attribute.PrimaryWeapon.Projectile.Shotgun" };
			inline static const ly::GameplayTag PelletCount{ "Attribute.PrimaryWeapon.Projectile.Shotgun.PelletCount" };
			inline static const ly::GameplayTag SpreadAngle{ "Attribute.PrimaryWeapon.Projectile.Shotgun.SpreadAngle" };
		};
	};

	struct Beam
	{
		inline static const ly::GameplayTag FamilyId{ "PrimaryWeapon.Beam" };

		struct Delivery
		{
			inline static const ly::GameplayTag AttributeRoot{ "Attribute.PrimaryWeapon.Beam.Delivery" };
			inline static const ly::GameplayTag Range{ "Attribute.PrimaryWeapon.Beam.Delivery.Range" };
			inline static const ly::GameplayTag Width{ "Attribute.PrimaryWeapon.Beam.Delivery.Width" };
		};

		struct Continuous
		{
			inline static const ly::GameplayTag TypeId{ "PrimaryWeapon.Beam.Continuous" };
		};
	};

	struct Feature
	{
		struct Heat
		{
			inline static const ly::GameplayTag FeatureId{ "PrimaryWeapon.Feature.Heat" };
			inline static const ly::GameplayTag AttributeRoot{ "Attribute.PrimaryWeapon.Feature.Heat" };
			inline static const ly::GameplayTag Gain{ "Attribute.PrimaryWeapon.Feature.Heat.Gain" };
			inline static const ly::GameplayTag Capacity{ "Attribute.PrimaryWeapon.Feature.Heat.Capacity" };
			inline static const ly::GameplayTag Dissipation{ "Attribute.PrimaryWeapon.Feature.Heat.Dissipation" };
			inline static const ly::GameplayTag CurrentRuntimeValue{ "Runtime.PrimaryWeapon.Feature.Heat.Current" };
		};
	};
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

struct PrimaryWeaponLevelStep
{
	ly::List<ly::AttributeModifier> modifiers;
};

struct PrimaryWeaponDefinition
{
	std::string weaponId;
	ly::GameplayTag weaponTypeTag;
	WeaponPresentationDefinition presentationDefinition;
	ly::GameplayAttributeList attributes;
	ly::List<WeaponMuzzleDefinition> muzzleDefinitions;
	bool automaticFire;
	ly::List<PrimaryWeaponLevelStep> levelProgression;
	ly::List<ly::AttributeModifier> attributeModifiers;
	ly::List<ly::AttributeScalingRule> scalingRules;
	ly::List<ly::GameplayTag> featureTags;

	PrimaryWeaponDefinition(
		const std::string& inWeaponId = "DefaultPrimaryWeapon",
		const ly::GameplayTag& inWeaponTypeTag = PrimaryWeaponSchema::Projectile::Standard::TypeId,
		const WeaponPresentationDefinition& inPresentationDefinition = WeaponPresentationDefinition{},
		const ly::GameplayAttributeList& inAttributes = {},
		const ly::List<WeaponMuzzleDefinition>& inMuzzleDefinitions = { WeaponMuzzleDefinition{} },
		bool inAutomaticFire = true,
		const ly::List<PrimaryWeaponLevelStep>& inLevelProgression = {},
		const ly::List<ly::AttributeModifier>& inAttributeModifiers = {},
		const ly::List<ly::AttributeScalingRule>& inScalingRules = {},
		const ly::List<ly::GameplayTag>& inFeatureTags = {}
	)
		: weaponId(inWeaponId)
		, weaponTypeTag(inWeaponTypeTag)
		, presentationDefinition(inPresentationDefinition)
		, attributes(inAttributes)
		, muzzleDefinitions(inMuzzleDefinitions)
		, automaticFire(inAutomaticFire)
		, levelProgression(inLevelProgression)
		, attributeModifiers(inAttributeModifiers)
		, scalingRules(inScalingRules)
		, featureTags(inFeatureTags)
	{
	}
};
