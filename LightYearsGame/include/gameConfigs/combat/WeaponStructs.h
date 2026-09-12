#pragma once

#include <SFML/Graphics.hpp>
#include "framework/Core.h"
#include "framework/MathUtility.h"
#include "attributes/AttributeSystem.h"
#include "engineConfigs/EngineStructs.h"
#include "gameplay/damage/DamageContext.h"
#include <string>
#include <optional>

enum class PrimaryWeaponType
{
	ProjectileStandard,
	ProjectileShotgun,
	ArcElectric,
	BeamContinuous,
	WaveExpanding
};

enum class PrimaryWeaponCadenceMode
{
	AuthoredScaling,
	OwnerAttackSpeedPercentage
};

enum class PrimaryWeaponDamageRoundingPolicy
{
	None,
	CeilFinalDamage
};

struct PrimaryWeaponMagazineDefinition
{
	int capacity = 1;
	float baseReloadTime = 1.f;

	bool operator==(const PrimaryWeaponMagazineDefinition& other) const
	{
		return capacity == other.capacity && baseReloadTime == other.baseReloadTime;
	}
	bool operator!=(const PrimaryWeaponMagazineDefinition& other) const
	{
		return !(*this == other);
	}
};

struct PrimaryWeaponEmpoweredShotDefinition
{
	bool guaranteedCritical = false;

	bool operator==(const PrimaryWeaponEmpoweredShotDefinition& other) const
	{
		return guaranteedCritical == other.guaranteedCritical;
	}
	bool operator!=(const PrimaryWeaponEmpoweredShotDefinition& other) const
	{
		return !(*this == other);
	}
};

struct PrimaryWeaponShotMetadata
{
	bool isEmpowered = false;
	ly::DamageCriticalPolicy criticalPolicy = ly::DamageCriticalPolicy::Random;
	bool roundFinalDamageUp = false;
};

enum class PrimaryWeaponFeatureType
{
	Heat
};

inline const char* PrimaryWeaponFeatureUpgradeId(PrimaryWeaponFeatureType type)
{
	return type == PrimaryWeaponFeatureType::Heat
		? "PrimaryWeapon.Feature.Heat"
		: "";
}

inline bool IsProjectileWeaponType(PrimaryWeaponType type)
{
	return type == PrimaryWeaponType::ProjectileStandard ||
		type == PrimaryWeaponType::ProjectileShotgun;
}

inline bool IsBeamWeaponType(PrimaryWeaponType type)
{
	return type == PrimaryWeaponType::BeamContinuous;
}

inline const char* PrimaryWeaponFamilyName(PrimaryWeaponType type)
{
	switch (type)
	{
	case PrimaryWeaponType::ProjectileStandard:
	case PrimaryWeaponType::ProjectileShotgun: return "Projectile";
	case PrimaryWeaponType::ArcElectric: return "Arc";
	case PrimaryWeaponType::BeamContinuous: return "Beam";
	case PrimaryWeaponType::WaveExpanding: return "Wave";
	}
	return "";
}

struct PrimaryWeaponSchema
{
	// A definition selects exactly one concrete leaf type, never a family tag.
	struct Projectile
	{
		struct Delivery
		{
			inline static const sas::AttributeId Root{ "PrimaryWeapon.Projectile.Delivery" };
			inline static const sas::AttributeId Speed{ "PrimaryWeapon.Projectile.Delivery.Speed" };
			inline static const sas::AttributeId Lifetime{ "PrimaryWeapon.Projectile.Delivery.Lifetime" };
			inline static const sas::AttributeId PierceCount{ "PrimaryWeapon.Projectile.Delivery.PierceCount" };
			inline static const sas::AttributeId AdditionalProjectileCount{
				"PrimaryWeapon.Projectile.Delivery.AdditionalProjectileCount"
			};
		};

		struct Standard
		{
		};

		struct Shotgun
		{
			inline static const sas::AttributeId Root{ "PrimaryWeapon.Projectile.Shotgun" };
			inline static const sas::AttributeId PelletCount{ "PrimaryWeapon.Projectile.Shotgun.PelletCount" };
			inline static const sas::AttributeId SpreadAngle{ "PrimaryWeapon.Projectile.Shotgun.SpreadAngle" };
			inline static const sas::AttributeId DamageReductionPerAdditionalHit{
				"PrimaryWeapon.Projectile.Shotgun.DamageReductionPerAdditionalHit"
			};
			inline static const sas::AttributeId MinimumDamageMultiplier{
				"PrimaryWeapon.Projectile.Shotgun.MinimumDamageMultiplier"
			};
		};

	};

	struct Arc
	{
		struct Electric
		{
			inline static const sas::AttributeId Root{ "PrimaryWeapon.Arc.Electric" };
			// Number of additional targets after the direct hit.
			inline static const sas::AttributeId ChainCount{ "PrimaryWeapon.Arc.Electric.ChainCount" };
			inline static const sas::AttributeId ChainRange{ "PrimaryWeapon.Arc.Electric.ChainRange" };
			inline static const sas::AttributeId DamageMultiplierPerChain{
				"PrimaryWeapon.Arc.Electric.DamageMultiplierPerChain"
			};
		};
	};

	struct Beam
	{
		struct Delivery
		{
			inline static const sas::AttributeId Root{ "PrimaryWeapon.Beam.Delivery" };
			inline static const sas::AttributeId Range{ "PrimaryWeapon.Beam.Delivery.Range" };
			inline static const sas::AttributeId Width{ "PrimaryWeapon.Beam.Delivery.Width" };
		};

		struct Continuous
		{
		};
	};

	struct Wave
	{
		struct Delivery
		{
			inline static const sas::AttributeId Root{ "PrimaryWeapon.Wave.Delivery" };
			inline static const sas::AttributeId Speed{ "PrimaryWeapon.Wave.Delivery.Speed" };
			inline static const sas::AttributeId InitialWidth{ "PrimaryWeapon.Wave.Delivery.InitialWidth" };
			inline static const sas::AttributeId MaximumWidth{ "PrimaryWeapon.Wave.Delivery.MaximumWidth" };
			inline static const sas::AttributeId Thickness{ "PrimaryWeapon.Wave.Delivery.Thickness" };
		};

		struct Expanding
		{
		};
	};

	struct Feature
	{
		struct Heat
		{
			inline static const sas::AttributeId Root{ "PrimaryWeapon.Feature.Heat" };
			inline static const sas::AttributeId Gain{ "PrimaryWeapon.Feature.Heat.Gain" };
			inline static const sas::AttributeId Capacity{ "PrimaryWeapon.Feature.Heat.Capacity" };
			inline static const sas::AttributeId Dissipation{ "PrimaryWeapon.Feature.Heat.Dissipation" };
			inline static const sas::AttributeId OverheatCooldown{
				"PrimaryWeapon.Feature.Heat.OverheatCooldown"
			};
			inline static const sas::AttributeId DamageMultiplierAtMaxHeat{
				"PrimaryWeapon.Feature.Heat.DamageMultiplierAtMaxHeat"
			};
			inline static const sas::AttributeId CurrentRuntimeValue{
				"PrimaryWeapon.Feature.Heat.Current"
			};
		};
	};

	struct Empowered
	{
		inline static const sas::AttributeId Root{ "PrimaryWeapon.Empowered" };
		inline static const sas::AttributeId BonusDamage{ "PrimaryWeapon.Empowered.BonusDamage" };
		inline static const sas::AttributeId EverySuccessfulShots{ "PrimaryWeapon.Empowered.EverySuccessfulShots" };
		inline static const sas::AttributeId FinalMagazineRounds{ "PrimaryWeapon.Empowered.FinalMagazineRounds" };
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

// Each segment ends at this percentage of the heat capacity. The lower bound is
// the preceding segment's end, or zero for the first segment.
struct HeatGainCurveSegmentDefinition
{
	float endHeatPercentage = 100.f;
	float gainMultiplier = 1.f;
};

struct PrimaryWeaponLevelStep
{
	ly::List<sas::AttributeModifier> attributeModifiers;
	ly::List<sas::AttributeScalingRule> scalingRules;
	ly::List<std::string> unlockedUpgradeIds;
	ly::List<PrimaryWeaponFeatureType> unlockedFeatureTypes;
};

// A rule applies its reward to every matching weapon level, inclusively.
// Weapon level one is the base definition; progression starts at level two.
struct WeaponLevelRule
{
	int firstLevel = 2;
	int lastLevel = 2;
	int levelInterval = 1;
	PrimaryWeaponLevelStep reward;

	bool AppliesAt(int weaponLevel) const
	{
		return weaponLevel >= firstLevel &&
			weaponLevel <= lastLevel &&
			levelInterval > 0 &&
			(weaponLevel - firstLevel) % levelInterval == 0;
	}
};

// Keeps weapon progression declarative without forcing a hand-written step for
// every level. Use EveryLevel for repeatable stat growth and AtLevel/BetweenLevels
// for milestones or temporary growth bands.
struct WeaponProgressionProfile
{
	int maxLevel = 1;
	ly::List<WeaponLevelRule> rules;
	// Indexed by target level minus two: [0] purchases level two.
	// An empty list keeps the profile usable for non-purchasable weapons.
	ly::List<unsigned int> levelUpgradeScrapCosts;

	WeaponProgressionProfile(int inMaxLevel = 1)
		: maxLevel(inMaxLevel)
	{
	}

	WeaponProgressionProfile& EveryLevel(
		const ly::List<sas::AttributeModifier>& modifiers,
		const ly::List<std::string>& upgradeIds = {},
		const ly::List<PrimaryWeaponFeatureType>& featureTypes = {},
		const ly::List<sas::AttributeScalingRule>& scalingRules = {}
	)
	{
		return BetweenLevels(2, maxLevel, modifiers, upgradeIds, featureTypes, 1, scalingRules);
	}

	WeaponProgressionProfile& ScrapCosts(const ly::List<unsigned int>& costs)
	{
		levelUpgradeScrapCosts = costs;
		return *this;
	}

	WeaponProgressionProfile& AtLevel(
		int level,
		const ly::List<sas::AttributeModifier>& modifiers = {},
		const ly::List<std::string>& upgradeIds = {},
		const ly::List<PrimaryWeaponFeatureType>& featureTypes = {},
		const ly::List<sas::AttributeScalingRule>& scalingRules = {}
	)
	{
		return BetweenLevels(level, level, modifiers, upgradeIds, featureTypes, 1, scalingRules);
	}

	WeaponProgressionProfile& BetweenLevels(
		int firstLevel,
		int lastLevel,
		const ly::List<sas::AttributeModifier>& modifiers = {},
		const ly::List<std::string>& upgradeIds = {},
		const ly::List<PrimaryWeaponFeatureType>& featureTypes = {},
		int levelInterval = 1,
		const ly::List<sas::AttributeScalingRule>& scalingRules = {}
	)
	{
		rules.push_back(WeaponLevelRule{
			firstLevel,
			lastLevel,
			levelInterval,
			PrimaryWeaponLevelStep{ modifiers, scalingRules, upgradeIds, featureTypes }
		});
		return *this;
	}

	WeaponProgressionProfile& FromLevel(
		int firstLevel,
		const ly::List<sas::AttributeModifier>& modifiers = {},
		const ly::List<std::string>& upgradeIds = {},
		const ly::List<PrimaryWeaponFeatureType>& featureTypes = {},
		int levelInterval = 1,
		const ly::List<sas::AttributeScalingRule>& scalingRules = {}
	)
	{
		return BetweenLevels(
			firstLevel,
			maxLevel,
			modifiers,
			upgradeIds,
			featureTypes,
			levelInterval,
			scalingRules
		);
	}

	bool IsWellFormed() const
	{
		if (maxLevel < 1)
		{
			return false;
		}
		for (const WeaponLevelRule& rule : rules)
		{
			if (rule.firstLevel < 2 ||
				rule.lastLevel < rule.firstLevel ||
				rule.lastLevel > maxLevel ||
				rule.levelInterval <= 0)
			{
				return false;
			}
		}
		if (!levelUpgradeScrapCosts.empty())
		{
			if (levelUpgradeScrapCosts.size() != static_cast<size_t>(maxLevel - 1))
			{
				return false;
			}
			for (const unsigned int cost : levelUpgradeScrapCosts)
			{
				if (cost == 0)
				{
					return false;
				}
			}
		}
		return true;
	}

	unsigned int GetScrapCostToReachLevel(int weaponLevel) const
	{
		if (weaponLevel < 2 || weaponLevel > maxLevel ||
			levelUpgradeScrapCosts.size() != static_cast<size_t>(maxLevel - 1))
		{
			return 0;
		}
		return levelUpgradeScrapCosts[static_cast<size_t>(weaponLevel - 2)];
	}

	ly::List<PrimaryWeaponLevelStep> ResolveLevelSteps() const
	{
		ly::List<PrimaryWeaponLevelStep> steps;
		if (!IsWellFormed() || maxLevel <= 1)
		{
			return steps;
		}

		steps.resize(static_cast<size_t>(maxLevel - 1));
		for (int level = 2; level <= maxLevel; ++level)
		{
			PrimaryWeaponLevelStep& resolvedStep = steps[static_cast<size_t>(level - 2)];
			for (const WeaponLevelRule& rule : rules)
			{
				if (!rule.AppliesAt(level))
				{
					continue;
				}
				resolvedStep.attributeModifiers.insert(
					resolvedStep.attributeModifiers.end(),
					rule.reward.attributeModifiers.begin(),
					rule.reward.attributeModifiers.end()
				);
				resolvedStep.scalingRules.insert(
					resolvedStep.scalingRules.end(),
					rule.reward.scalingRules.begin(),
					rule.reward.scalingRules.end()
				);
				resolvedStep.unlockedUpgradeIds.insert(
					resolvedStep.unlockedUpgradeIds.end(),
					rule.reward.unlockedUpgradeIds.begin(),
					rule.reward.unlockedUpgradeIds.end()
				);
				resolvedStep.unlockedFeatureTypes.insert(
					resolvedStep.unlockedFeatureTypes.end(),
					rule.reward.unlockedFeatureTypes.begin(),
					rule.reward.unlockedFeatureTypes.end()
				);
			}
		}
		return steps;
	}
};

struct PrimaryWeaponDefinition
{
	std::string weaponId;
	PrimaryWeaponType weaponType = PrimaryWeaponType::ProjectileStandard;
	WeaponPresentationDefinition presentationDefinition;
	sas::GameplayAttributeList attributes;
	ly::List<WeaponMuzzleDefinition> muzzleDefinitions;
	bool automaticFire;
	WeaponProgressionProfile progressionProfile;
	ly::List<sas::AttributeModifier> attributeModifiers;
	ly::List<sas::AttributeScalingRule> scalingRules;
	ly::List<PrimaryWeaponFeatureType> featureTypes;
	ly::List<HeatGainCurveSegmentDefinition> heatGainCurve;
	ly::List<ly::GameplayTag> damageTags;
	ly::List<ly::GameplayTag> attachmentCapabilities;
	size_t attachmentSlotCapacity = 2;
	std::optional<PrimaryWeaponMagazineDefinition> magazine;
	PrimaryWeaponCadenceMode cadenceMode = PrimaryWeaponCadenceMode::AuthoredScaling;
	PrimaryWeaponDamageRoundingPolicy damageRoundingPolicy = PrimaryWeaponDamageRoundingPolicy::None;
	std::optional<PrimaryWeaponEmpoweredShotDefinition> empoweredShot;

	PrimaryWeaponDefinition(
		// Empty identifies an ephemeral/test definition. Shipped catalog entries
		// always receive a validated Weapon.* content ID from WeaponLoader.
		const std::string& inWeaponId = {},
		PrimaryWeaponType inWeaponType = PrimaryWeaponType::ProjectileStandard,
		const WeaponPresentationDefinition& inPresentationDefinition = WeaponPresentationDefinition{},
		const sas::GameplayAttributeList& inAttributes = {},
		const ly::List<WeaponMuzzleDefinition>& inMuzzleDefinitions = { WeaponMuzzleDefinition{} },
		bool inAutomaticFire = true,
		const WeaponProgressionProfile& inProgressionProfile = WeaponProgressionProfile{},
		const ly::List<sas::AttributeModifier>& inAttributeModifiers = {},
		const ly::List<sas::AttributeScalingRule>& inScalingRules = {},
		const ly::List<PrimaryWeaponFeatureType>& inFeatureTypes = {},
		const ly::List<HeatGainCurveSegmentDefinition>& inHeatGainCurve = {},
		const ly::List<ly::GameplayTag>& inDamageTags = {},
		const ly::List<ly::GameplayTag>& inAttachmentCapabilities = {},
		size_t inAttachmentSlotCapacity = 2,
		const std::optional<PrimaryWeaponMagazineDefinition>& inMagazine = std::nullopt,
		PrimaryWeaponCadenceMode inCadenceMode = PrimaryWeaponCadenceMode::AuthoredScaling,
		PrimaryWeaponDamageRoundingPolicy inDamageRoundingPolicy = PrimaryWeaponDamageRoundingPolicy::None,
		const std::optional<PrimaryWeaponEmpoweredShotDefinition>& inEmpoweredShot = std::nullopt
	)
		: weaponId(inWeaponId)
		, weaponType(inWeaponType)
		, presentationDefinition(inPresentationDefinition)
		, attributes(inAttributes)
		, muzzleDefinitions(inMuzzleDefinitions)
		, automaticFire(inAutomaticFire)
		, progressionProfile(inProgressionProfile)
		, attributeModifiers(inAttributeModifiers)
		, scalingRules(inScalingRules)
		, featureTypes(inFeatureTypes)
		, heatGainCurve(inHeatGainCurve)
		, damageTags(inDamageTags)
		, attachmentCapabilities(inAttachmentCapabilities)
		, attachmentSlotCapacity(inAttachmentSlotCapacity)
		, magazine(inMagazine)
		, cadenceMode(inCadenceMode)
		, damageRoundingPolicy(inDamageRoundingPolicy)
		, empoweredShot(inEmpoweredShot)
	{
	}
};
