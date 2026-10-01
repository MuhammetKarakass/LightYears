#pragma once

#include "framework/JsonDocumentLoader.h"
#include "gameConfigs/combat/ScrapCostFormula.h"
#include <limits>
#include <stdexcept>
#include <string>

namespace ly
{
	// Reads progression.scrapCost = { "base": n, "step": n }. An absent key
	// leaves the progression unpurchasable; the retired per-level
	// "levelUpgradeScrapCosts" list is rejected so stale data cannot go unnoticed.
	inline ScrapCostFormula ParseScrapCostFormula(
		const JsonDocumentLoader::Json& progression,
		const std::string& ownerLabel
	)
	{
		const std::string context = ownerLabel + ": progression.scrapCost";
		if (progression.contains("levelUpgradeScrapCosts"))
		{
			throw std::runtime_error(
				ownerLabel + ": progression.levelUpgradeScrapCosts is retired; use progression.scrapCost {base, step}"
			);
		}
		ScrapCostFormula formula;
		if (!progression.contains("scrapCost"))
		{
			return formula;
		}
		const auto& object = progression.at("scrapCost");
		if (!object.is_object())
		{
			throw std::runtime_error(context + " must be an object");
		}
		const auto readField = [&](const char* key, bool required) -> unsigned int
		{
			if (!object.contains(key))
			{
				if (required)
				{
					throw std::runtime_error(context + "." + key + " is required");
				}
				return 0u;
			}
			const auto& value = object.at(key);
			if (!value.is_number_integer() || value.get<long long>() < 0 ||
				value.get<long long>() > static_cast<long long>(std::numeric_limits<unsigned int>::max()))
			{
				throw std::runtime_error(context + "." + key + " must be a non-negative integer");
			}
			return value.get<unsigned int>();
		};
		formula.base = readField("base", true);
		formula.step = readField("step", false);
		if (formula.base == 0)
		{
			throw std::runtime_error(context + ".base must be greater than zero");
		}
		return formula;
	}
}
