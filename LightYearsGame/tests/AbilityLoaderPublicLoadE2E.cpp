#include "gameConfigs/ability/AbilityCatalog.h"
#include "gameplay/content/AbilityContentCatalog.h"
#include "gameplay/content/GameContentBootstrap.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace ly
{
	namespace
	{
		using Json = nlohmann::json;

		constexpr const char* ShippedAbilityId = "Ability.Movement.EnergySpear.Basic";

		bool WriteJsonFile(const std::filesystem::path& path, const Json& document)
		{
			std::ofstream output{ path, std::ios::out | std::ios::trunc };
			if (!output)
			{
				std::cerr << "Could not open ability loader fixture: " << path.string() << '\n';
				return false;
			}
			output << document.dump(2) << '\n';
			return output.good();
		}

		bool ReloadShippedDefinitions(
			const std::filesystem::path& path,
			std::string* failureReason
		)
		{
			return content::AbilityContentCatalog::LoadFromFile(
				path,
				AbilityData::GetBuiltinShippedAbilityDefinitions(),
				AbilityData::GetBuiltinAbilityActorDefinitions(),
				failureReason
			);
		}

		bool CatalogIsFailClosed()
		{
			return !content::AbilityContentCatalog::IsLoaded() &&
				content::AbilityContentCatalog::GetDefinitions().empty() &&
				content::AbilityContentCatalog::FindById(ShippedAbilityId) == nullptr;
		}
	}

	int RunAbilityLoaderPublicLoadE2E(const char* artifactPath, const std::string& setupError)
	{
		const std::filesystem::path shippedPath =
			std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } /
			"LightYearsGame/assets/content/data/abilities.json";
		const std::filesystem::path artifact = std::filesystem::absolute(artifactPath);
		const std::filesystem::path fixtureDirectory = artifact.parent_path() / "ability-loader-fixtures";
		const std::filesystem::path missingFieldPath = fixtureDirectory / "missing-cooldown.json";
		const std::filesystem::path invalidFieldPath = fixtureDirectory / "invalid-cooldown-type.json";
		const std::filesystem::path missingFilePath = fixtureDirectory / "absent-document.json";
		std::error_code directoryError;
		std::filesystem::create_directories(fixtureDirectory, directoryError);
		if (directoryError)
		{
			std::cerr << "Could not create ability loader fixture directory: " << directoryError.message() << '\n';
			return 1;
		}

		Json sourceDocument;
		std::string fixtureError;
		{
			std::ifstream input{ shippedPath, std::ios::in };
			if (!input)
			{
				fixtureError = "Could not open shipped ability JSON";
			}
			else
			{
				try
				{
					input >> sourceDocument;
				}
				catch (const std::exception& exception)
				{
					fixtureError = exception.what();
				}
			}
		}
		if (fixtureError.empty() && (!sourceDocument.contains("abilities") || !sourceDocument["abilities"].is_array()))
		{
			fixtureError = "Shipped ability JSON has no abilities array";
		}
		if (fixtureError.empty())
		{
			bool targetAbilityFound = false;
			for (Json& ability : sourceDocument["abilities"])
			{
				if (ability.value("id", std::string{}) != ShippedAbilityId)
				{
					continue;
				}
				targetAbilityFound = true;
				Json missingFieldDocument = sourceDocument;
				for (Json& candidate : missingFieldDocument["abilities"])
				{
					if (candidate.value("id", std::string{}) == ShippedAbilityId)
					{
						candidate.erase("cooldown");
						break;
					}
				}
				if (!WriteJsonFile(missingFieldPath, missingFieldDocument))
				{
					fixtureError = "Could not write missing-field fixture";
				}

				Json invalidFieldDocument = sourceDocument;
				for (Json& candidate : invalidFieldDocument["abilities"])
				{
					if (candidate.value("id", std::string{}) == ShippedAbilityId)
					{
						candidate["cooldown"] = "invalid";
						break;
					}
				}
				if (fixtureError.empty() && !WriteJsonFile(invalidFieldPath, invalidFieldDocument))
				{
					fixtureError = "Could not write invalid-field fixture";
				}
				break;
			}
			if (!targetAbilityFound)
			{
				fixtureError = "Shipped Energy Spear ability is absent from JSON";
			}
		}

		const std::size_t expectedDefinitionCount = sourceDocument.contains("abilities") &&
			sourceDocument["abilities"].is_array() ? sourceDocument["abilities"].size() : 0;
		std::string validLoadFailure;
		const bool validLoadSucceeded = fixtureError.empty() && ReloadShippedDefinitions(shippedPath, &validLoadFailure);
		const bool validLoadCatalogReady = validLoadSucceeded &&
			content::AbilityContentCatalog::IsLoaded() &&
			content::AbilityContentCatalog::GetDefinitions().size() == expectedDefinitionCount &&
			content::AbilityContentCatalog::FindById(ShippedAbilityId) != nullptr;

		std::string missingFieldFailure;
		const bool missingFieldLoadSucceeded = fixtureError.empty() &&
			ReloadShippedDefinitions(missingFieldPath, &missingFieldFailure);
		const bool missingFieldRejected = !missingFieldLoadSucceeded &&
			missingFieldFailure.find("cooldown") != std::string::npos;
		const bool missingFieldFailureClearedCatalog = CatalogIsFailClosed();

		std::string recoveredAfterMissingFailure;
		const bool recoveredAfterMissing = fixtureError.empty() &&
			ReloadShippedDefinitions(shippedPath, &recoveredAfterMissingFailure) &&
			content::AbilityContentCatalog::IsLoaded() &&
			content::AbilityContentCatalog::FindById(ShippedAbilityId) != nullptr;

		std::string invalidFieldFailure;
		const bool invalidFieldLoadSucceeded = fixtureError.empty() &&
			ReloadShippedDefinitions(invalidFieldPath, &invalidFieldFailure);
		const bool invalidFieldRejected = !invalidFieldLoadSucceeded &&
			invalidFieldFailure.find("cooldown") != std::string::npos;
		const bool invalidFieldFailureClearedCatalog = CatalogIsFailClosed();

		std::string recoveredAfterInvalidFailure;
		const bool recoveredAfterInvalid = fixtureError.empty() &&
			ReloadShippedDefinitions(shippedPath, &recoveredAfterInvalidFailure) &&
			content::AbilityContentCatalog::IsLoaded() &&
			content::AbilityContentCatalog::FindById(ShippedAbilityId) != nullptr;

		std::string missingFileFailure;
		const bool missingFileLoadSucceeded = fixtureError.empty() &&
			ReloadShippedDefinitions(missingFilePath, &missingFileFailure);
		const bool missingFileRejected = !missingFileLoadSucceeded && !missingFileFailure.empty();
		const bool missingFileFailureClearedCatalog = CatalogIsFailClosed();

		std::string recoveredAfterMissingFileFailure;
		const bool recoveredAfterMissingFile = fixtureError.empty() &&
			ReloadShippedDefinitions(shippedPath, &recoveredAfterMissingFileFailure) &&
			content::AbilityContentCatalog::IsLoaded() &&
			content::AbilityContentCatalog::FindById(ShippedAbilityId) != nullptr;

		const bool bootstrapSucceeded = setupError.empty();
		const bool fixtureCreationSucceeded = fixtureError.empty();
		const bool passed = bootstrapSucceeded && fixtureCreationSucceeded && validLoadCatalogReady &&
			missingFieldRejected && missingFieldFailureClearedCatalog && recoveredAfterMissing &&
			invalidFieldRejected && invalidFieldFailureClearedCatalog && recoveredAfterInvalid &&
			missingFileRejected && missingFileFailureClearedCatalog && recoveredAfterMissingFile;

		nlohmann::json result{
			{ "assertions", {
				{ "validShippedJsonLoadsThroughPublicCatalog", validLoadCatalogReady },
				{ "missingCooldownFieldRejected", missingFieldRejected },
				{ "missingFieldFailureClearsCatalog", missingFieldFailureClearedCatalog },
				{ "catalogRecoversAfterMissingFieldFailure", recoveredAfterMissing },
				{ "nonnumericCooldownRejected", invalidFieldRejected },
				{ "invalidFieldFailureClearsCatalog", invalidFieldFailureClearedCatalog },
				{ "catalogRecoversAfterInvalidFieldFailure", recoveredAfterInvalid },
				{ "absentFileReloadRejected", missingFileRejected },
				{ "absentFileFailureClearsCatalog", missingFileFailureClearedCatalog },
				{ "catalogRecoversAfterAbsentFileFailure", recoveredAfterMissingFile }
			} },
			{ "input", {
				{ "catalogEntryPoint", "AbilityContentCatalog::LoadFromFile" },
				{ "expectedDefinitionCount", expectedDefinitionCount },
				{ "expectedDefinitionId", ShippedAbilityId },
				{ "invalidFieldFixture", "ability-loader-fixtures/invalid-cooldown-type.json" },
				{ "missingFieldFixture", "ability-loader-fixtures/missing-cooldown.json" },
				{ "missingFileFixture", "ability-loader-fixtures/absent-document.json" },
				{ "shippedJson", "LightYearsGame/assets/content/data/abilities.json" }
			} },
			{ "outcome", {
				{ "abilityCatalogLoadedAtEnd", content::AbilityContentCatalog::IsLoaded() },
				{ "definitionCountAtInitialLoad", validLoadCatalogReady ? expectedDefinitionCount : 0 },
				{ "failureReasonsReported", {
					{ "invalidCooldown", !invalidFieldFailure.empty() },
					{ "missingCooldown", !missingFieldFailure.empty() },
					{ "missingFile", !missingFileFailure.empty() }
				} },
				{ "fixtureError", fixtureError },
				{ "setupError", setupError }
			} },
			{ "passed", passed },
			{ "scenario", "d2.ability_loader_public_load_and_failed_reload" },
			{ "schemaVersion", 1 }
		};

		if (!WriteJsonFile(artifact, result))
		{
			return 1;
		}
		std::cout << "E2E artifact: " << artifact.string() << '\n';
		if (!passed)
		{
			std::cerr << "Ability loader public-load E2E failed; see the artifact for outcomes.\n";
			return 1;
		}
		std::cout << "Ability loader public-load E2E passed.\n";
		return 0;
	}
}
