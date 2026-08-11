#include "gameplay/content/WeaponLoader.h"

#include "attributes/AttributeId.h"
#include "gameplay/content/ContentIdSchema.h"

#include "framework/JsonDocumentLoader.h"

#include <cstdint>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

namespace ly::content
{
	namespace
	{
		using Json = JsonDocumentLoader::Json;

		std::string RequiredString(const Json& object, const char* fieldName)
		{
			return object.at(fieldName).get<std::string>();
		}

		sf::Vector2f ParseVector2(const Json& value)
		{
			if (!value.is_array() || value.size() != 2)
			{
				throw std::runtime_error("Expected a two-element vector");
			}
			return {
				value.at(0).get<float>(),
				value.at(1).get<float>()
			};
		}

		sf::Color ParseColor(const Json& value)
		{
			if (!value.is_array() || value.size() != 4)
			{
				throw std::runtime_error("Expected a four-element color");
			}
			return {
				value.at(0).get<std::uint8_t>(),
				value.at(1).get<std::uint8_t>(),
				value.at(2).get<std::uint8_t>(),
				value.at(3).get<std::uint8_t>()
			};
		}

		sas::AttributeModifierOperation ParseOperation(const std::string& value)
		{
			if (value == "Add")
			{
				return sas::AttributeModifierOperation::Add;
			}
			if (value == "Multiply")
			{
				return sas::AttributeModifierOperation::Multiply;
			}
			if (value == "Override")
			{
				return sas::AttributeModifierOperation::Override;
			}
			throw std::runtime_error("Unknown weapon modifier operation: " + value);
		}

		sas::GameplayAttribute ParseAttribute(const Json& object)
		{
			return sas::GameplayAttribute{
				sas::AttributeId{ RequiredString(object, "id") },
				object.at("baseValue").get<float>(),
				object.value("minValue", 0.f),
				object.value(
					"maxValue",
					std::numeric_limits<float>::max()
				)
			};
		}

		sas::AttributeModifier ParseModifier(const Json& object)
		{
			return sas::AttributeModifier{
				sas::AttributeId{ RequiredString(object, "attributeId") },
				ParseOperation(RequiredString(object, "operation")),
				object.at("magnitude").get<float>(),
				object.value("priority", 0)
			};
		}

		ly::List<sas::AttributeModifier> ParseModifiers(const Json& values)
		{
			ly::List<sas::AttributeModifier> modifiers;
			for (const Json& value : values)
			{
				modifiers.emplace_back(ParseModifier(value));
			}
			return modifiers;
		}

		ly::List<std::string> ParseUpgradeIds(const Json& values)
		{
			ly::List<std::string> ids;
			for (const Json& value : values)
			{
				ids.emplace_back(value.get<std::string>());
			}
			return ids;
		}

		ly::List<GameplayTag> ParseTags(const Json& values)
		{
			ly::List<GameplayTag> tags;
			for (const Json& value : values)
			{
				tags.emplace_back(GameplayTag{ value.get<std::string>() });
			}
			return tags;
		}

		PrimaryWeaponType ParseWeaponType(const std::string& value)
		{
			if (value == "PrimaryWeapon.Projectile.Standard") return PrimaryWeaponType::ProjectileStandard;
			if (value == "PrimaryWeapon.Projectile.Shotgun") return PrimaryWeaponType::ProjectileShotgun;
			if (value == "PrimaryWeapon.Arc.Electric") return PrimaryWeaponType::ArcElectric;
			if (value == "PrimaryWeapon.Beam.Continuous") return PrimaryWeaponType::BeamContinuous;
			if (value == "PrimaryWeapon.Wave.Expanding") return PrimaryWeaponType::WaveExpanding;
			throw std::runtime_error("Unknown primary weapon type: " + value);
		}

		PrimaryWeaponFeatureType ParseFeatureType(const std::string& value)
		{
			if (value == "PrimaryWeapon.Feature.Heat") return PrimaryWeaponFeatureType::Heat;
			throw std::runtime_error("Unknown primary weapon feature: " + value);
		}

		ly::List<PrimaryWeaponFeatureType> ParseFeatureTypes(const Json& values)
		{
			ly::List<PrimaryWeaponFeatureType> types;
			for (const Json& value : values)
			{
				types.push_back(ParseFeatureType(value.get<std::string>()));
			}
			return types;
		}

		WeaponPresentationDefinition ParsePresentation(const Json& object)
		{
			const Json& pointLight = object.at("pointLight");
			const Json& color = pointLight.at("color");
			const PointLightDefinition pointLightDefinition{
				RequiredString(pointLight, "shaderPath"),
				ParseColor(color),
				pointLight.at("intensity").get<float>(),
				ParseVector2(pointLight.at("size")),
				pointLight.at("shouldStretch").get<bool>(),
				pointLight.at("complexTrail").get<bool>(),
				pointLight.value("taperAmount", 0.f),
				pointLight.value("edgeSoftness", 1.f),
				pointLight.value("shapeRoundness", 1.f)
			};

			return WeaponPresentationDefinition{
				RequiredString(object, "texturePath"),
				pointLightDefinition,
				ParseVector2(object.at("lightOffset")),
				object.value("visualScale", 1.f)
			};
		}

		WeaponMuzzleDefinition ParseMuzzle(const Json& object)
		{
			return WeaponMuzzleDefinition{
				ParseVector2(object.at("offset")),
				object.value("rotationOffset", 0.f)
			};
		}

		PrimaryWeaponLevelStep ParseLevelStep(const Json& object)
		{
			return PrimaryWeaponLevelStep{
				ParseModifiers(object.value("attributeModifiers", Json::array())),
				ParseUpgradeIds(object.value("unlockedUpgradeIds", Json::array())),
				ParseFeatureTypes(object.value("unlockedFeatureTags", Json::array()))
			};
		}

		WeaponProgressionProfile ParseProgression(const Json& object)
		{
			WeaponProgressionProfile profile{
				object.value("maxLevel", 1)
			};
			profile.levelUpgradeScrapCosts = object.value(
				"levelUpgradeScrapCosts",
				ly::List<unsigned int>{}
			);

			for (const Json& rule : object.value("rules", Json::array()))
			{
				profile.rules.emplace_back(WeaponLevelRule{
					rule.value("firstLevel", 2),
					rule.value("lastLevel", 2),
					rule.value("levelInterval", 1),
					ParseLevelStep(rule.at("reward"))
				});
			}
			return profile;
		}

		PrimaryWeaponDefinition ParseWeapon(const Json& object)
		{
			PrimaryWeaponDefinition definition;
			definition.weaponId = RequiredString(object, "id");
			std::string weaponIdFailure;
			if (!ContentIdSchema::ValidateWeaponId(definition.weaponId, &weaponIdFailure))
			{
				throw std::runtime_error(weaponIdFailure);
			}
			definition.weaponType = ParseWeaponType(RequiredString(object, "typeTag"));
			definition.presentationDefinition = ParsePresentation(
				object.at("presentation")
			);

			for (const Json& attribute : object.at("attributes"))
			{
				definition.attributes.emplace_back(ParseAttribute(attribute));
			}
			definition.muzzleDefinitions.clear();
			for (const Json& muzzle : object.at("muzzles"))
			{
				definition.muzzleDefinitions.emplace_back(ParseMuzzle(muzzle));
			}

			definition.automaticFire = object.value("automaticFire", true);
			definition.progressionProfile = ParseProgression(
				object.value("progression", Json::object())
			);
			definition.attributeModifiers = ParseModifiers(
				object.value("attributeModifiers", Json::array())
			);

			for (const Json& scalingRule : object.value("scalingRules", Json::array()))
			{
				definition.scalingRules.emplace_back(sas::AttributeScalingRule{
					sas::AttributeId{ RequiredString(scalingRule, "targetAttributeId") },
					sas::AttributeId{ RequiredString(scalingRule, "sourceAttributeId") },
					ParseOperation(RequiredString(scalingRule, "operation")),
					scalingRule.value("coefficient", 1.f)
				});
			}

			definition.featureTypes = ParseFeatureTypes(
				object.value("featureTags", Json::array())
			);
			for (const Json& segment : object.value("heatGainCurve", Json::array()))
			{
				definition.heatGainCurve.emplace_back(
					HeatGainCurveSegmentDefinition{
						segment.value("endHeatPercentage", 100.f),
						segment.value("gainMultiplier", 1.f)
					}
				);
			}
			definition.damageTags = ParseTags(
				object.value("damageTags", Json::array())
			);
			definition.attachmentCapabilities = ParseTags(
				object.value("attachmentCapabilities", Json::array())
			);
			definition.attachmentSlotCapacity = object.value(
				"attachmentSlotCapacity",
				static_cast<size_t>(2)
			);

			return definition;
		}
	}

	WeaponLoader::Result WeaponLoader::LoadFromFile(
		const std::filesystem::path& filePath)
	{
		const JsonDocumentLoader::Result documentResult =
			JsonDocumentLoader::LoadFromFile(filePath);
		if (!documentResult.Succeeded())
		{
			return Result{ {}, documentResult.error };
		}

		try
		{
			const Json& root = *documentResult.document;
			if (root.at("schemaVersion").get<int>() != 1)
			{
				return Result{ {}, "Unsupported weapon schema version" };
			}

			Result result;
			std::set<std::string> weaponIds;
			for (const Json& weapon : root.at("weapons"))
			{
				PrimaryWeaponDefinition definition = ParseWeapon(weapon);
				if (definition.weaponId.empty())
				{
					throw std::runtime_error("Weapon ID cannot be empty");
				}
				if (!weaponIds.insert(definition.weaponId).second)
				{
					throw std::runtime_error(
						"Duplicate weapon ID: " + definition.weaponId
					);
				}
				result.definitions.emplace_back(std::move(definition));
			}
			return result;
		}
		catch (const std::exception& exception)
		{
			return Result{
				{},
				"Failed to load weapons from '" +
				filePath.string() +
				"': " +
				exception.what()
			};
		}
	}
}
