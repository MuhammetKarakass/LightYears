#include "gameplay/weapon/PrimaryWeaponValidationContract.h"

#include "gameplay/attributes/AttributeIds.h"

#include <cmath>

namespace ly
{
	namespace
	{
		const sas::GameplayAttribute* FindDefinitionAttribute(const PrimaryWeaponDefinition& definition, const sas::AttributeId& attributeId)
		{
			return sas::FindAttribute(definition.attributes, attributeId);
		}

		PrimaryWeaponValidationResult RequireAttribute(const PrimaryWeaponDefinition& definition, const sas::AttributeId& attributeId, const char* ownerName)
		{
			return FindDefinitionAttribute(definition, attributeId)
				? PrimaryWeaponValidationResult{ true, {} }
				: PrimaryWeaponValidationResult{ false, std::string{ ownerName } + " requires attribute '" + std::string{ attributeId.GetName() } + "'." };
		}

		PrimaryWeaponValidationResult ValidateStandardProjectile(const PrimaryWeaponDefinition& definition)
		{
			for (const sas::AttributeId& required : {
				CommonAttributeIds::Damage,
				PrimaryWeaponSchema::Projectile::Delivery::Speed,
				PrimaryWeaponSchema::Projectile::Delivery::Lifetime
			})
			{
				const PrimaryWeaponValidationResult result = RequireAttribute(definition, required, "Projectile weapon");
				if (!result.isValid)
				{
					return result;
				}
			}
			return { true, {} };
		}

		PrimaryWeaponValidationResult ValidateShotgun(const PrimaryWeaponDefinition& definition)
		{
			for (const sas::AttributeId& required : {
				CommonAttributeIds::Damage,
				PrimaryWeaponSchema::Projectile::Delivery::Speed,
				PrimaryWeaponSchema::Projectile::Delivery::Lifetime,
				PrimaryWeaponSchema::Projectile::Shotgun::PelletCount,
				PrimaryWeaponSchema::Projectile::Shotgun::SpreadAngle
			})
			{
				const PrimaryWeaponValidationResult result = RequireAttribute(definition, required, "Shotgun weapon");
				if (!result.isValid)
				{
					return result;
				}
			}

			const float pelletCount = FindDefinitionAttribute(definition, PrimaryWeaponSchema::Projectile::Shotgun::PelletCount)->baseValue;
			if (pelletCount < 2.f || std::round(pelletCount) != pelletCount)
			{
				return { false, "Shotgun pellet count must be an integer of at least two." };
			}
			if (FindDefinitionAttribute(definition, PrimaryWeaponSchema::Projectile::Shotgun::SpreadAngle)->baseValue < 0.f)
			{
				return { false, "Shotgun spread angle cannot be negative." };
			}

			const sas::GameplayAttribute* damageReduction = FindDefinitionAttribute(
				definition, PrimaryWeaponSchema::Projectile::Shotgun::DamageReductionPerAdditionalHit);
			const sas::GameplayAttribute* minimumMultiplier = FindDefinitionAttribute(
				definition, PrimaryWeaponSchema::Projectile::Shotgun::MinimumDamageMultiplier);
			if (!damageReduction && !minimumMultiplier)
			{
				return { true, {} };
			}
			if (!damageReduction || !minimumMultiplier)
			{
				return { false, "Shotgun pellet falloff requires both reduction and minimum multiplier attributes." };
			}
			if (damageReduction->baseValue <= 0.f || damageReduction->baseValue >= 1.f)
			{
				return { false, "Shotgun pellet damage reduction must be between zero and one." };
			}
			if (minimumMultiplier->baseValue <= 0.f || minimumMultiplier->baseValue > 1.f)
			{
				return { false, "Shotgun minimum damage multiplier must be greater than zero and at most one." };
			}
			if (sas::FindAttributeValue(definition.attributes, AreaAttributeIds::Radius, 0.f) > 0.f ||
				sas::FindAttributeValue(definition.attributes, PrimaryWeaponSchema::Projectile::Delivery::PierceCount, 0.f) > 0.f)
			{
				return { false, "Shotgun pellet falloff only supports direct, non-piercing pellets." };
			}
			return { true, {} };
		}

		PrimaryWeaponValidationResult ValidateElectricArc(const PrimaryWeaponDefinition& definition)
		{
			for (const sas::AttributeId& required : {
				CommonAttributeIds::Damage,
				CommonAttributeIds::Range,
				PrimaryWeaponSchema::Arc::Electric::ChainCount,
				PrimaryWeaponSchema::Arc::Electric::ChainRange,
				PrimaryWeaponSchema::Arc::Electric::DamageMultiplierPerChain
			})
			{
				const PrimaryWeaponValidationResult result = RequireAttribute(definition, required, "Electric arc weapon");
				if (!result.isValid)
				{
					return result;
				}
			}

			const float chainCount = FindDefinitionAttribute(definition, PrimaryWeaponSchema::Arc::Electric::ChainCount)->baseValue;
			if (chainCount < 0.f || std::round(chainCount) != chainCount)
			{
				return { false, "Arc chain count must be a non-negative integer." };
			}
			const float chainRange = FindDefinitionAttribute(definition, PrimaryWeaponSchema::Arc::Electric::ChainRange)->baseValue;
			if (FindDefinitionAttribute(definition, CommonAttributeIds::Range)->baseValue <= 0.f || chainRange <= 0.f)
			{
				return { false, "Electric arc target and chain ranges must be greater than zero." };
			}
			const float damageMultiplier = FindDefinitionAttribute(
				definition, PrimaryWeaponSchema::Arc::Electric::DamageMultiplierPerChain)->baseValue;
			return damageMultiplier > 0.f && damageMultiplier <= 1.f
				? PrimaryWeaponValidationResult{ true, {} }
				: PrimaryWeaponValidationResult{ false, "Arc damage multiplier per chain must be greater than zero and at most one." };
		}

		PrimaryWeaponValidationResult ValidateContinuousBeam(const PrimaryWeaponDefinition& definition)
		{
			for (const sas::AttributeId& required : {
				CommonAttributeIds::Damage,
				PrimaryWeaponSchema::Beam::Delivery::Range,
				PrimaryWeaponSchema::Beam::Delivery::Width
			})
			{
				const PrimaryWeaponValidationResult result = RequireAttribute(definition, required, "Continuous beam weapon");
				if (!result.isValid)
				{
					return result;
				}
			}

			const float range = FindDefinitionAttribute(definition, PrimaryWeaponSchema::Beam::Delivery::Range)->baseValue;
			const float width = FindDefinitionAttribute(definition, PrimaryWeaponSchema::Beam::Delivery::Width)->baseValue;
			return range > 0.f && width > 0.f
				? PrimaryWeaponValidationResult{ true, {} }
				: PrimaryWeaponValidationResult{ false, "Continuous beam range and width must be greater than zero." };
		}

		PrimaryWeaponValidationResult ValidateExpandingWave(const PrimaryWeaponDefinition& definition)
		{
			for (const sas::AttributeId& required : {
				CommonAttributeIds::Damage,
				CommonAttributeIds::Range,
				PrimaryWeaponSchema::Wave::Delivery::Speed,
				PrimaryWeaponSchema::Wave::Delivery::InitialWidth,
				PrimaryWeaponSchema::Wave::Delivery::MaximumWidth,
				PrimaryWeaponSchema::Wave::Delivery::Thickness
			})
			{
				const PrimaryWeaponValidationResult result = RequireAttribute(definition, required, "Expanding wave weapon");
				if (!result.isValid)
				{
					return result;
				}
			}

			const float initialWidth = sas::FindAttributeValue(
				definition.attributes, PrimaryWeaponSchema::Wave::Delivery::InitialWidth, 0.f);
			const float maximumWidth = sas::FindAttributeValue(
				definition.attributes, PrimaryWeaponSchema::Wave::Delivery::MaximumWidth, 0.f);
			if (initialWidth <= 0.f || maximumWidth < initialWidth)
			{
				return { false, "Expanding wave weapon requires a positive initial width and a maximum width no smaller than it." };
			}
			return { true, {} };
		}

		PrimaryWeaponValidationResult ValidateHeat(const PrimaryWeaponDefinition& definition)
		{
			for (const sas::AttributeId& required : {
				PrimaryWeaponSchema::Feature::Heat::Gain,
				PrimaryWeaponSchema::Feature::Heat::Capacity,
				PrimaryWeaponSchema::Feature::Heat::Dissipation
			})
			{
				const PrimaryWeaponValidationResult result = RequireAttribute(definition, required, "Heat feature");
				if (!result.isValid)
				{
					return result;
				}
			}
			const sas::GameplayAttribute* capacity = FindDefinitionAttribute(
				definition, PrimaryWeaponSchema::Feature::Heat::Capacity);
			if (!capacity || capacity->baseValue <= 0.f)
			{
				return { false, "Heat capacity must be greater than zero." };
			}
			if (!definition.heatGainCurve.empty())
			{
				float previousEndPercentage = 0.f;
				for (const HeatGainCurveSegmentDefinition& segment : definition.heatGainCurve)
				{
					if (segment.endHeatPercentage <= previousEndPercentage || segment.endHeatPercentage > 100.f || segment.gainMultiplier <= 0.f)
					{
						return { false, "Heat gain curve segments must have increasing end percentages within zero to one hundred and positive multipliers." };
					}
					previousEndPercentage = segment.endHeatPercentage;
				}
				if (std::abs(previousEndPercentage - 100.f) > 0.001f)
				{
					return { false, "Heat gain curve must end at one hundred percent heat." };
				}
			}

			if (definition.weaponType != PrimaryWeaponType::BeamContinuous)
			{
				return { true, {} };
			}
			for (const sas::AttributeId& required : {
				PrimaryWeaponSchema::Feature::Heat::OverheatCooldown,
				PrimaryWeaponSchema::Feature::Heat::DamageMultiplierAtMaxHeat
			})
			{
				const PrimaryWeaponValidationResult result = RequireAttribute(definition, required, "Continuous beam heat feature");
				if (!result.isValid)
				{
					return result;
				}
			}
			const sas::GameplayAttribute* cooldown = FindDefinitionAttribute(
				definition, PrimaryWeaponSchema::Feature::Heat::OverheatCooldown);
			const sas::GameplayAttribute* maximumDamageMultiplier = FindDefinitionAttribute(
				definition, PrimaryWeaponSchema::Feature::Heat::DamageMultiplierAtMaxHeat);
			return cooldown->baseValue > 0.f && maximumDamageMultiplier->baseValue >= 1.f
				? PrimaryWeaponValidationResult{ true, {} }
				: PrimaryWeaponValidationResult{ false, "Continuous beam heat cooldown must be positive and its maximum damage multiplier must be at least one." };
		}

		const PrimaryWeaponTypeValidationContract& StandardProjectileContract()
		{
			static const PrimaryWeaponTypeValidationContract contract{
				PrimaryWeaponType::ProjectileStandard,
				{ PrimaryWeaponSchema::Projectile::Delivery::Root },
				{},
				true,
				&ValidateStandardProjectile
			};
			return contract;
		}

		const PrimaryWeaponTypeValidationContract& ShotgunContract()
		{
			static const PrimaryWeaponTypeValidationContract contract{
				PrimaryWeaponType::ProjectileShotgun,
				{ PrimaryWeaponSchema::Projectile::Shotgun::Root },
				{ PrimaryWeaponSchema::Projectile::Delivery::Root },
				true,
				&ValidateShotgun
			};
			return contract;
		}

		const PrimaryWeaponTypeValidationContract& ElectricArcContract()
		{
			static const PrimaryWeaponTypeValidationContract contract{
				PrimaryWeaponType::ArcElectric,
				{ PrimaryWeaponSchema::Arc::Electric::Root },
				{},
				true,
				&ValidateElectricArc
			};
			return contract;
		}

		const PrimaryWeaponTypeValidationContract& ContinuousBeamContract()
		{
			static const PrimaryWeaponTypeValidationContract contract{
				PrimaryWeaponType::BeamContinuous,
				{ PrimaryWeaponSchema::Beam::Delivery::Root },
				{},
				false,
				&ValidateContinuousBeam
			};
			return contract;
		}

		const PrimaryWeaponTypeValidationContract& ExpandingWaveContract()
		{
			static const PrimaryWeaponTypeValidationContract contract{
				PrimaryWeaponType::WaveExpanding,
				{ PrimaryWeaponSchema::Wave::Delivery::Root },
				{},
				true,
				&ValidateExpandingWave
			};
			return contract;
		}

		const PrimaryWeaponFeatureValidationContract& HeatContract()
		{
			static const PrimaryWeaponFeatureValidationContract contract{
				PrimaryWeaponFeatureType::Heat,
				{ PrimaryWeaponSchema::Feature::Heat::Root },
				&ValidateHeat
			};
			return contract;
		}
	}

	const PrimaryWeaponTypeValidationContract* PrimaryWeaponValidationContractRegistry::FindType(PrimaryWeaponType weaponType)
	{
		const PrimaryWeaponTypeValidationContract* contract = nullptr;
		switch (weaponType)
		{
		case PrimaryWeaponType::ProjectileStandard: contract = &StandardProjectileContract(); break;
		case PrimaryWeaponType::ProjectileShotgun: contract = &ShotgunContract(); break;
		case PrimaryWeaponType::ArcElectric: contract = &ElectricArcContract(); break;
		case PrimaryWeaponType::BeamContinuous: contract = &ContinuousBeamContract(); break;
		case PrimaryWeaponType::WaveExpanding: contract = &ExpandingWaveContract(); break;
		default: return nullptr;
		}
		return contract->weaponType == weaponType ? contract : nullptr;
	}

	const PrimaryWeaponFeatureValidationContract* PrimaryWeaponValidationContractRegistry::FindFeature(PrimaryWeaponFeatureType featureType)
	{
		const PrimaryWeaponFeatureValidationContract* contract = nullptr;
		switch (featureType)
		{
		case PrimaryWeaponFeatureType::Heat: contract = &HeatContract(); break;
		default: return nullptr;
		}
		return contract->featureType == featureType ? contract : nullptr;
	}
}
