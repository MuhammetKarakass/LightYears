#pragma once

#include <SFML/Graphics.hpp>
#include "framework/Core.h"
#include "framework/MathUtility.h"
#include "attributes/AttributeSystem.h"
#include "engineConfigs/EngineStructs.h"
#include "gameplay/damage/DamageContext.h"
#include <string>
#include <optional>
#include <numeric>
#include <algorithm>

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

inline const char* PrimaryWeaponTypeName(PrimaryWeaponType type)
{
	switch (type)
	{
	case PrimaryWeaponType::ProjectileStandard: return "ProjectileStandard";
	case PrimaryWeaponType::ProjectileShotgun: return "ProjectileShotgun";
	case PrimaryWeaponType::ArcElectric: return "ArcElectric";
	case PrimaryWeaponType::BeamContinuous: return "BeamContinuous";
	case PrimaryWeaponType::WaveExpanding: return "WaveExpanding";
	}
	return "Unknown";
}

inline const char* PrimaryWeaponFeatureName(PrimaryWeaponFeatureType type)
{
	return type == PrimaryWeaponFeatureType::Heat ? "Heat" : "Unknown";
}

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

// lastLevel == kOpenEndedWeaponLevel means the rule keeps applying forever.
inline constexpr int kOpenEndedWeaponLevel = 0;
inline constexpr int kMaxWeaponCycleLength = 64;

// A rule applies its reward to every matching weapon level, inclusively.
// Weapon level one is the base definition; progression starts at level two.
struct WeaponLevelRule
{
	int firstLevel = 2;
	int lastLevel = kOpenEndedWeaponLevel;
	int levelInterval = 1;
	PrimaryWeaponLevelStep reward;

	bool IsOpenEnded() const { return lastLevel == kOpenEndedWeaponLevel; }

	bool AppliesAt(int weaponLevel) const
	{
		return weaponLevel >= firstLevel &&
			(IsOpenEnded() || weaponLevel <= lastLevel) &&
			levelInterval > 0 &&
			(weaponLevel - firstLevel) % levelInterval == 0;
	}
};

// Weapon progression is unbounded: level N re-applies per-level rewards forever.
// Steps are an authored prefix (index 0 reaches level two) plus a cycle that
// repeats after the prefix (cycle[0] is the first level after the prefix).
struct ResolvedWeaponProgression
{
	ly::List<PrimaryWeaponLevelStep> prefix;
	ly::List<PrimaryWeaponLevelStep> cycle;
};

// Keeps weapon progression declarative without forcing a hand-written step for
// every level. Use EveryLevel for repeatable stat growth and AtLevel/BetweenLevels
// for milestones or temporary growth bands.
struct WeaponProgressionProfile
{
	ly::List<WeaponLevelRule> rules;
	// Indexed by target level minus two: [0] purchases level two. Levels beyond
	// the list cost 0 and remain purchasable; an empty list is not purchasable.
	ly::List<unsigned int> levelUpgradeScrapCosts;

	WeaponProgressionProfile& EveryLevel(
		const ly::List<sas::AttributeModifier>& modifiers,
		const ly::List<std::string>& upgradeIds = {},
		const ly::List<PrimaryWeaponFeatureType>& featureTypes = {},
		const ly::List<sas::AttributeScalingRule>& scalingRules = {}
	)
	{
		return BetweenLevels(2, kOpenEndedWeaponLevel, modifiers, upgradeIds, featureTypes, 1, scalingRules);
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

	// lastLevel == kOpenEndedWeaponLevel makes the band open-ended.
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
			kOpenEndedWeaponLevel,
			modifiers,
			upgradeIds,
			featureTypes,
			levelInterval,
			scalingRules
		);
	}

	bool IsWellFormed() const
	{
		int cycleLength = 1;
		for (const WeaponLevelRule& rule : rules)
		{
			if (rule.firstLevel < 2 ||
				rule.levelInterval <= 0 ||
				(!rule.IsOpenEnded() && rule.lastLevel < rule.firstLevel))
			{
				return false;
			}
			if (rule.IsOpenEnded())
			{
				cycleLength = std::lcm(cycleLength, rule.levelInterval);
				if (cycleLength > kMaxWeaponCycleLength)
				{
					return false;
				}
			}
		}
		for (const unsigned int cost : levelUpgradeScrapCosts)
		{
			if (cost == 0)
			{
				return false;
			}
		}
		return true;
	}

	// Authored cost, or 0 for levels beyond the list (still purchasable).
	unsigned int GetScrapCostToReachLevel(int weaponLevel) const
	{
		if (weaponLevel < 2)
		{
			return 0;
		}
		const size_t index = static_cast<size_t>(weaponLevel - 2);
		return index < levelUpgradeScrapCosts.size() ? levelUpgradeScrapCosts[index] : 0;
	}

	// Every distinct step (prefix followed by one cycle); for validators/scanners.
	ly::List<PrimaryWeaponLevelStep> ResolveDistinctSteps() const
	{
		ResolvedWeaponProgression progression = ResolveProgression();
		progression.prefix.insert(
			progression.prefix.end(), progression.cycle.begin(), progression.cycle.end());
		return progression.prefix;
	}

	// prefix covers levels 2..P where P = max(highest bounded lastLevel, highest
	// open-ended firstLevel - 1). Past P only open-ended rules apply, and each one
	// repeats with its interval, so the rewards are periodic with C = lcm(intervals)
	// (<= kMaxWeaponCycleLength). cycle[i] holds the rewards for level P + 1 + i.
	ResolvedWeaponProgression ResolveProgression() const
	{
		ResolvedWeaponProgression result;
		if (!IsWellFormed())
		{
			return result;
		}

		int prefixEnd = 1;
		int cycleLength = 1;
		bool hasOpenRule = false;
		for (const WeaponLevelRule& rule : rules)
		{
			if (rule.IsOpenEnded())
			{
				hasOpenRule = true;
				prefixEnd = std::max(prefixEnd, rule.firstLevel - 1);
				cycleLength = std::lcm(cycleLength, rule.levelInterval);
			}
			else
			{
				prefixEnd = std::max(prefixEnd, rule.lastLevel);
			}
		}

		const auto resolveLevel = [this](int level)
		{
			PrimaryWeaponLevelStep resolvedStep;
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
			return resolvedStep;
		};

		for (int level = 2; level <= prefixEnd; ++level)
		{
			result.prefix.push_back(resolveLevel(level));
		}
		if (hasOpenRule)
		{
			const int firstCycleLevel = std::max(prefixEnd, 1) + 1;
			for (int i = 0; i < cycleLength; ++i)
			{
				result.cycle.push_back(resolveLevel(firstCycleLevel + i));
			}
		}
		return result;
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
