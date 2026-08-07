#pragma once

#include <SFML/Graphics.hpp>
#include "framework/Core.h"
#include "framework/MathUtility.h"
#include "attributes/AttributeSystem.h"
#include "engineConfigs/EngineStructs.h"
#include <string>

struct PrimaryWeaponSchema
{
	// A definition selects exactly one concrete leaf type, never a family tag.
	struct Projectile
	{
		inline static const ly::GameplayTag FamilyTag{ "PrimaryWeapon.Projectile" };

		struct Delivery
		{
			inline static const ly::GameplayTag AttributeRoot{ "Attribute.PrimaryWeapon.Projectile.Delivery" };
			inline static const ly::GameplayTag Speed{ "Attribute.PrimaryWeapon.Projectile.Delivery.Speed" };
			inline static const ly::GameplayTag Lifetime{ "Attribute.PrimaryWeapon.Projectile.Delivery.Lifetime" };
			inline static const ly::GameplayTag PierceCount{ "Attribute.PrimaryWeapon.Projectile.Delivery.PierceCount" };
			inline static const ly::GameplayTag AdditionalProjectileCount{
				"Attribute.PrimaryWeapon.Projectile.Delivery.AdditionalProjectileCount"
			};
		};

		struct Standard
		{
			inline static const ly::GameplayTag TypeTag{ "PrimaryWeapon.Projectile.Standard" };
		};

		struct Shotgun
		{
			inline static const ly::GameplayTag TypeTag{ "PrimaryWeapon.Projectile.Shotgun" };
			inline static const ly::GameplayTag AttributeRoot{ "Attribute.PrimaryWeapon.Projectile.Shotgun" };
			inline static const ly::GameplayTag PelletCount{ "Attribute.PrimaryWeapon.Projectile.Shotgun.PelletCount" };
			inline static const ly::GameplayTag SpreadAngle{ "Attribute.PrimaryWeapon.Projectile.Shotgun.SpreadAngle" };
			inline static const ly::GameplayTag DamageReductionPerAdditionalHit{
				"Attribute.PrimaryWeapon.Projectile.Shotgun.DamageReductionPerAdditionalHit"
			};
			inline static const ly::GameplayTag MinimumDamageMultiplier{
				"Attribute.PrimaryWeapon.Projectile.Shotgun.MinimumDamageMultiplier"
			};
		};

	};

	struct Arc
	{
		inline static const ly::GameplayTag FamilyTag{ "PrimaryWeapon.Arc" };

		struct Electric
		{
			inline static const ly::GameplayTag TypeTag{ "PrimaryWeapon.Arc.Electric" };
			inline static const ly::GameplayTag AttributeRoot{ "Attribute.PrimaryWeapon.Arc.Electric" };
			// Number of additional targets after the direct hit.
			inline static const ly::GameplayTag ChainCount{ "Attribute.PrimaryWeapon.Arc.Electric.ChainCount" };
			inline static const ly::GameplayTag ChainRange{ "Attribute.PrimaryWeapon.Arc.Electric.ChainRange" };
			inline static const ly::GameplayTag DamageMultiplierPerChain{
				"Attribute.PrimaryWeapon.Arc.Electric.DamageMultiplierPerChain"
			};
		};
	};

	struct Beam
	{
		inline static const ly::GameplayTag FamilyTag{ "PrimaryWeapon.Beam" };

		struct Delivery
		{
			inline static const ly::GameplayTag AttributeRoot{ "Attribute.PrimaryWeapon.Beam.Delivery" };
			inline static const ly::GameplayTag Range{ "Attribute.PrimaryWeapon.Beam.Delivery.Range" };
			inline static const ly::GameplayTag Width{ "Attribute.PrimaryWeapon.Beam.Delivery.Width" };
		};

		struct Continuous
		{
			inline static const ly::GameplayTag TypeTag{ "PrimaryWeapon.Beam.Continuous" };
		};
	};

	struct Wave
	{
		inline static const ly::GameplayTag FamilyTag{ "PrimaryWeapon.Wave" };

		struct Delivery
		{
			inline static const ly::GameplayTag AttributeRoot{ "Attribute.PrimaryWeapon.Wave.Delivery" };
			inline static const ly::GameplayTag Speed{ "Attribute.PrimaryWeapon.Wave.Delivery.Speed" };
			inline static const ly::GameplayTag InitialWidth{ "Attribute.PrimaryWeapon.Wave.Delivery.InitialWidth" };
			inline static const ly::GameplayTag MaximumWidth{ "Attribute.PrimaryWeapon.Wave.Delivery.MaximumWidth" };
			inline static const ly::GameplayTag Thickness{ "Attribute.PrimaryWeapon.Wave.Delivery.Thickness" };
		};

		struct Expanding
		{
			inline static const ly::GameplayTag TypeTag{ "PrimaryWeapon.Wave.Expanding" };
		};
	};

	struct Feature
	{
		struct Heat
		{
			inline static const ly::GameplayTag FeatureTag{ "PrimaryWeapon.Feature.Heat" };
			inline static const ly::GameplayTag AttributeRoot{ "Attribute.PrimaryWeapon.Feature.Heat" };
			inline static const ly::GameplayTag Gain{ "Attribute.PrimaryWeapon.Feature.Heat.Gain" };
			inline static const ly::GameplayTag Capacity{ "Attribute.PrimaryWeapon.Feature.Heat.Capacity" };
			inline static const ly::GameplayTag Dissipation{ "Attribute.PrimaryWeapon.Feature.Heat.Dissipation" };
			inline static const ly::GameplayTag OverheatCooldown{
				"Attribute.PrimaryWeapon.Feature.Heat.OverheatCooldown"
			};
			inline static const ly::GameplayTag DamageMultiplierAtMaxHeat{
				"Attribute.PrimaryWeapon.Feature.Heat.DamageMultiplierAtMaxHeat"
			};
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
	ly::List<ly::GameplayTag> unlockedUpgradeIds;
	ly::List<ly::GameplayTag> unlockedFeatureTags;
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
		const ly::List<ly::GameplayTag>& upgradeIds = {},
		const ly::List<ly::GameplayTag>& featureTags = {}
	)
	{
		return BetweenLevels(2, maxLevel, modifiers, upgradeIds, featureTags);
	}

	WeaponProgressionProfile& ScrapCosts(const ly::List<unsigned int>& costs)
	{
		levelUpgradeScrapCosts = costs;
		return *this;
	}

	WeaponProgressionProfile& AtLevel(
		int level,
		const ly::List<sas::AttributeModifier>& modifiers = {},
		const ly::List<ly::GameplayTag>& upgradeIds = {},
		const ly::List<ly::GameplayTag>& featureTags = {}
	)
	{
		return BetweenLevels(level, level, modifiers, upgradeIds, featureTags);
	}

	WeaponProgressionProfile& BetweenLevels(
		int firstLevel,
		int lastLevel,
		const ly::List<sas::AttributeModifier>& modifiers = {},
		const ly::List<ly::GameplayTag>& upgradeIds = {},
		const ly::List<ly::GameplayTag>& featureTags = {},
		int levelInterval = 1
	)
	{
		rules.push_back(WeaponLevelRule{
			firstLevel,
			lastLevel,
			levelInterval,
			PrimaryWeaponLevelStep{ modifiers, upgradeIds, featureTags }
		});
		return *this;
	}

	WeaponProgressionProfile& FromLevel(
		int firstLevel,
		const ly::List<sas::AttributeModifier>& modifiers = {},
		const ly::List<ly::GameplayTag>& upgradeIds = {},
		const ly::List<ly::GameplayTag>& featureTags = {},
		int levelInterval = 1
	)
	{
		return BetweenLevels(
			firstLevel,
			maxLevel,
			modifiers,
			upgradeIds,
			featureTags,
			levelInterval
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
				resolvedStep.unlockedUpgradeIds.insert(
					resolvedStep.unlockedUpgradeIds.end(),
					rule.reward.unlockedUpgradeIds.begin(),
					rule.reward.unlockedUpgradeIds.end()
				);
				resolvedStep.unlockedFeatureTags.insert(
					resolvedStep.unlockedFeatureTags.end(),
					rule.reward.unlockedFeatureTags.begin(),
					rule.reward.unlockedFeatureTags.end()
				);
			}
		}
		return steps;
	}
};

struct PrimaryWeaponDefinition
{
	std::string weaponId;
	ly::GameplayTag weaponTypeTag;
	WeaponPresentationDefinition presentationDefinition;
	sas::GameplayAttributeList attributes;
	ly::List<WeaponMuzzleDefinition> muzzleDefinitions;
	bool automaticFire;
	WeaponProgressionProfile progressionProfile;
	ly::List<sas::AttributeModifier> attributeModifiers;
	ly::List<sas::AttributeScalingRule> scalingRules;
	ly::List<ly::GameplayTag> featureTags;
	ly::List<HeatGainCurveSegmentDefinition> heatGainCurve;
	ly::List<ly::GameplayTag> damageTags;
	ly::List<ly::GameplayTag> attachmentCapabilities;
	size_t attachmentSlotCapacity = 2;

	PrimaryWeaponDefinition(
		// Empty identifies an ephemeral/test definition. Shipped catalog entries
		// always receive a validated Weapon.* content ID from WeaponLoader.
		const std::string& inWeaponId = {},
		const ly::GameplayTag& inWeaponTypeTag = PrimaryWeaponSchema::Projectile::Standard::TypeTag,
		const WeaponPresentationDefinition& inPresentationDefinition = WeaponPresentationDefinition{},
		const sas::GameplayAttributeList& inAttributes = {},
		const ly::List<WeaponMuzzleDefinition>& inMuzzleDefinitions = { WeaponMuzzleDefinition{} },
		bool inAutomaticFire = true,
		const WeaponProgressionProfile& inProgressionProfile = WeaponProgressionProfile{},
		const ly::List<sas::AttributeModifier>& inAttributeModifiers = {},
		const ly::List<sas::AttributeScalingRule>& inScalingRules = {},
		const ly::List<ly::GameplayTag>& inFeatureTags = {},
		const ly::List<HeatGainCurveSegmentDefinition>& inHeatGainCurve = {},
		const ly::List<ly::GameplayTag>& inDamageTags = {},
		const ly::List<ly::GameplayTag>& inAttachmentCapabilities = {},
		size_t inAttachmentSlotCapacity = 2
	)
		: weaponId(inWeaponId)
		, weaponTypeTag(inWeaponTypeTag)
		, presentationDefinition(inPresentationDefinition)
		, attributes(inAttributes)
		, muzzleDefinitions(inMuzzleDefinitions)
		, automaticFire(inAutomaticFire)
		, progressionProfile(inProgressionProfile)
		, attributeModifiers(inAttributeModifiers)
		, scalingRules(inScalingRules)
		, featureTags(inFeatureTags)
		, heatGainCurve(inHeatGainCurve)
		, damageTags(inDamageTags)
		, attachmentCapabilities(inAttachmentCapabilities)
		, attachmentSlotCapacity(inAttachmentSlotCapacity)
	{
	}
};
