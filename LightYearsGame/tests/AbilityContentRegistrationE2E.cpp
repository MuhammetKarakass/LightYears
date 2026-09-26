#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/content/AbilityBehaviorType.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/energySpear/EnergySpearAbility.h"
#include "gameplay/content/AbilityContentCatalog.h"
#include "gameplay/content/GameContentBootstrap.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/energySpear/EnergySpearPresentationIds.h"
#include "presentation/ability/energySpear/EnergySpearPresentationProfile.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace ly
{
	namespace
	{
		constexpr const char* ShippedEnergySpearAbilityId = "Ability.Movement.EnergySpear.Basic";

		bool WriteArtifact(const std::filesystem::path& path, const nlohmann::json& artifact)
		{
			std::error_code error;
			if (!path.parent_path().empty())
			{
				std::filesystem::create_directories(path.parent_path(), error);
				if (error)
				{
					std::cerr << "Could not create E2E artifact directory: " << error.message() << '\n';
					return false;
				}
			}

			std::ofstream output{ path, std::ios::out | std::ios::trunc };
			if (!output)
			{
				std::cerr << "Could not open E2E artifact path: " << path.string() << '\n';
				return false;
			}
			output << artifact.dump(2) << '\n';
			return output.good();
		}
	}

	int RunAbilityContentRegistrationE2E(const char* artifactPath, const std::string& setupError)
	{
		const GameAbilityDefinition* definition =
			content::AbilityContentCatalog::FindById(ShippedEnergySpearAbilityId);
		const bool catalogLoaded = content::AbilityContentCatalog::IsLoaded();
		const bool definitionFound = definition != nullptr;
		const bool behaviorTypeMatches = definitionFound &&
			definition->behaviorType == AbilityBehaviorType::EnergySpear;
		unique_ptr<GameAbilityBehavior> behavior = behaviorTypeMatches
			? GameAbilityBehaviorRegistry::Create(definition->behaviorType)
			: nullptr;
		const bool factoryCreatedExpectedBehavior =
			dynamic_cast<EnergySpearAbility*>(behavior.get()) != nullptr;
		std::string behaviorValidationFailure;
		const bool behaviorValidated = factoryCreatedExpectedBehavior &&
			behavior->Validate(*definition, &behaviorValidationFailure);

		const EnergySpearPresentationProfile* chargeProfile =
			PresentationProfileRegistry<EnergySpearPresentationProfile>::Find(
				EnergySpearPresentationIds::ChargeBasic
			);
		const EnergySpearPresentationProfile* traversalProfile =
			PresentationProfileRegistry<EnergySpearPresentationProfile>::Find(
				EnergySpearPresentationIds::TraversalBasic
			);
		const bool chargeProfileRegistered = chargeProfile &&
			chargeProfile->profileId.ToString() == EnergySpearPresentationIds::ChargeBasic;
		const bool traversalProfileRegistered = traversalProfile &&
			traversalProfile->profileId.ToString() == EnergySpearPresentationIds::TraversalBasic;
		const bool bootstrapSucceeded = setupError.empty();
		const bool passed = bootstrapSucceeded && catalogLoaded && definitionFound &&
			behaviorTypeMatches && factoryCreatedExpectedBehavior && behaviorValidated &&
			chargeProfileRegistered && traversalProfileRegistered;

		nlohmann::json artifact{
			{ "assertions", {
				{ "bootstrapRegistrationAndShippedValidationSucceeded", bootstrapSucceeded },
				{ "chargeProfileRegisteredInTypedRegistry", chargeProfileRegistered },
				{ "energySpearFactoryCreatesExpectedBehavior", factoryCreatedExpectedBehavior },
				{ "shippedDefinitionUsesEnergySpearBehavior", behaviorTypeMatches },
				{ "shippedEnergySpearDefinitionLoaded", definitionFound },
				{ "traversalProfileRegisteredInTypedRegistry", traversalProfileRegistered },
				{ "shippedDefinitionPassesBehaviorValidation", behaviorValidated }
			} },
			{ "input", {
				{ "abilityDataPath", "LightYearsGame/assets/content/data/abilities.json" },
				{ "abilityId", ShippedEnergySpearAbilityId },
				{ "expectedBehaviorType", "EnergySpear" },
				{ "presentationProfileIds", {
					{ "charge", EnergySpearPresentationIds::ChargeBasic },
					{ "traversal", EnergySpearPresentationIds::TraversalBasic }
				} },
				{ "registrationEntryPoint", "GameContentBootstrap::Register" }
			} },
			{ "outcome", {
				{ "abilityCatalogLoaded", catalogLoaded },
				{ "bootstrapSucceeded", bootstrapSucceeded },
				{ "behaviorValidationFailure", behaviorValidationFailure },
				{ "shippedDefinitionBehaviorType", behaviorTypeMatches ? "EnergySpear" : "unknown" }
			} },
			{ "passed", passed },
			{ "scenario", "d2.energy_spear_family_registration" },
			{ "schemaVersion", 1 },
			{ "setupError", setupError }
		};

		if (!WriteArtifact(artifactPath, artifact))
		{
			return 1;
		}
		std::cout << "E2E artifact: " << std::filesystem::absolute(artifactPath).string() << '\n';
		if (!passed)
		{
			std::cerr << "Ability content registration E2E failed; see the artifact for outcomes.\n";
			return 1;
		}
		std::cout << "Ability content registration E2E passed.\n";
		return 0;
	}
}
