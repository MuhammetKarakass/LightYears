#include "gameplay/content/DamageStatusBalanceLoader.h"

#include "framework/JsonDocumentLoader.h"

#include <cmath>
#include <set>
#include <stdexcept>

namespace ly::content
{
	namespace
	{
		using Json = JsonDocumentLoader::Json;

		void ValidateKeys(
			const Json& object,
			const std::set<std::string>& allowedKeys,
			const std::string& context
		)
		{
			if (!object.is_object())
			{
				throw std::runtime_error(context + " must be an object");
			}
			for (auto it = object.begin(); it != object.end(); ++it)
			{
				if (allowedKeys.find(it.key()) == allowedKeys.end())
				{
					throw std::runtime_error(
						"Unknown key '" + it.key() + "' in " + context
					);
				}
			}
		}

		float ParseFiniteNonNegative(
			const Json& object,
			const char* fieldName,
			const std::string& context
		)
		{
			if (!object.contains(fieldName) || !object.at(fieldName).is_number())
			{
				throw std::runtime_error(
					context + " requires numeric field '" + fieldName + "'"
				);
			}
			const float value = object.at(fieldName).get<float>();
			if (!std::isfinite(value) || value < 0.f)
			{
				throw std::runtime_error(
					context + " field '" + fieldName + "' must be finite and non-negative"
				);
			}
			return value;
		}

		DamageStatusFamilyBalance ParseFamily(
			const Json& object,
			const char* familyName,
			bool percentageValues
		)
		{
			const std::string context = "damage status family '" + std::string{ familyName } + "'";
			ValidateKeys(object, { "duration", "maxStacks", "values" }, context);
			DamageStatusFamilyBalance family;
			family.duration = ParseFiniteNonNegative(object, "duration", context);
			if (family.duration <= 0.f)
			{
				throw std::runtime_error(context + " duration must be positive");
			}
			if (!object.contains("maxStacks") || !object.at("maxStacks").is_number_integer())
			{
				throw std::runtime_error(context + " requires integer field 'maxStacks'");
			}
			family.maxStacks = object.at("maxStacks").get<int>();
			if (family.maxStacks != static_cast<int>(DamageStatusFamilyBalance::ValueCount))
			{
				throw std::runtime_error(
					context + " maxStacks must equal " +
					std::to_string(DamageStatusFamilyBalance::ValueCount)
				);
			}
			if (!object.contains("values") || !object.at("values").is_array() ||
				object.at("values").size() != static_cast<std::size_t>(family.maxStacks))
			{
				throw std::runtime_error(
					context + " values length must equal maxStacks"
				);
			}
			for (std::size_t index = 0; index < object.at("values").size(); ++index)
			{
				const Json& valueJson = object.at("values").at(index);
				if (!valueJson.is_number())
				{
					throw std::runtime_error(
						context + " values must contain only numbers"
					);
				}
				const float value = valueJson.get<float>();
				if (!std::isfinite(value) || value < 0.f ||
					(percentageValues && value > 1.f))
				{
					throw std::runtime_error(
						context + " values contain an out-of-range number"
					);
				}
				family.values[index] = value;
			}
			return family;
		}

		DamageStatusBalance ParseBalance(const Json& root)
		{
			ValidateKeys(
				root,
				{ "schemaVersion", "energyShieldDamageMultiplier", "statuses" },
				"damage status balance root"
			);
			if (!root.contains("schemaVersion") || !root.at("schemaVersion").is_number_integer() ||
				root.at("schemaVersion").get<int>() != 1)
			{
				throw std::runtime_error("Unsupported damage status balance schema version");
			}

			DamageStatusBalance balance;
			balance.energyShieldDamageMultiplier = ParseFiniteNonNegative(
				root,
				"energyShieldDamageMultiplier",
				"damage status balance root"
			);
			if (!root.contains("statuses"))
			{
				throw std::runtime_error("damage status balance root requires 'statuses'");
			}
			const Json& statuses = root.at("statuses");
			ValidateKeys(statuses, { "Cryo", "Electric", "Thermal", "Kinetic" }, "statuses");
			for (const char* familyName : { "Cryo", "Electric", "Thermal", "Kinetic" })
			{
				if (!statuses.contains(familyName))
				{
					throw std::runtime_error(
						"statuses requires damage family '" + std::string{ familyName } + "'"
					);
				}
			}
			balance.cryo = ParseFamily(statuses.at("Cryo"), "Cryo", true);
			balance.electric = ParseFamily(statuses.at("Electric"), "Electric", true);
			balance.thermal = ParseFamily(statuses.at("Thermal"), "Thermal", false);
			balance.kinetic = ParseFamily(statuses.at("Kinetic"), "Kinetic", true);
			return balance;
		}
	}

	DamageStatusBalanceLoader::Result DamageStatusBalanceLoader::LoadFromFile(
		const std::filesystem::path& filePath
	)
	{
		const JsonDocumentLoader::Result documentResult =
			JsonDocumentLoader::LoadFromFile(filePath);
		if (!documentResult.Succeeded())
		{
			return Result{ {}, documentResult.error };
		}

		try
		{
			return Result{ ParseBalance(*documentResult.document), {} };
		}
		catch (const std::exception& exception)
		{
			return Result{
				{},
				"Failed to load damage status balance from '" +
				filePath.string() + "': " + exception.what()
			};
		}
	}
}
