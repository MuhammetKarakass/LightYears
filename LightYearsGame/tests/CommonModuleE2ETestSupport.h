#pragma once

#include "framework/Application.h"
#include "framework/AssetManager.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/content/GameAbilityActions.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/content/GameContentBootstrap.h"
#include "gameplay/content/ModuleContentCatalog.h"
#include "gameplay/content/WeaponContentCatalog.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "gameplay/module/ModuleLoadout.h"
#include "gameplay/module/ModulePlacementRules.h"
#include "gameplay/module/ModuleResolver.h"
#include "gameplay/weapon/PrimaryWeaponExecutionSystem.h"
#include "gameplay/weapon/projectile/PrimaryWeaponProjectileActor.h"
#include "gameplay/tags/GameplayTags.h"
#include "level/GameLevel.h"
#include "player/PlayerSpaceShip.h"
#include "presentation/hud/modules/ModuleScreenPresenter.h"
#include "widget/TextWidget.h"
#include "widget/UIStyle.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace common_module_e2e
{
	using Json = nlohmann::json;

	class HeadlessModuleLevel final : public vega::GameLevel
	{
	public:
		explicit HeadlessModuleLevel(vega::Application* application) : GameLevel{ application } {}

	protected:
		void InitializeLevelSystems() override {}
		void CreateGameHUD() override {}
		void CreateHUDControllers() override {}
		void OnGameStart() override {}
	};

	struct Report
	{
		Json artifact{ { "test", "CommonModuleE2E" }, { "passed", false }, { "checks", Json::array() } };

		void Check(bool passed, const std::string& name, Json actual = nullptr)
		{
			Json check{ { "name", name }, { "passed", passed } };
			if (!actual.is_null()) check["actual"] = std::move(actual);
			artifact["checks"].push_back(std::move(check));
			if (!passed) throw std::runtime_error{ name };
		}

		void CheckValue(const std::string& name, float expected, float actual, const std::string& input)
		{
			Json check{ { "name", name }, { "passed", std::abs(expected - actual) < 0.001f },
				{ "input", input }, { "expected", expected }, { "actual", actual } };
			artifact["checks"].push_back(std::move(check));
			if (!artifact["checks"].back()["passed"].get<bool>()) throw std::runtime_error{ name };
		}
	};

	inline bool NearlyEqual(float left, float right)
	{
		return std::abs(left - right) < 0.001f;
	}

	inline const vega::ModuleDefinition* FindModule(const vega::ModuleRulesContext& rules, const char* moduleId)
	{
		return rules.lookup ? rules.lookup(moduleId) : nullptr;
	}

	inline std::optional<std::pair<sf::Vector2i, int>> FindPlacement(
		const vega::ModuleRulesContext& rules,
		const vega::ModuleLoadout& loadout,
		const vega::ModuleDefinition& module,
		std::uint32_t instanceId,
		std::uint32_t ignoredInstanceId = 0,
		std::optional<sf::Vector2i> excludedOrigin = std::nullopt
	)
	{
		vega::ModulePlacement candidate;
		candidate.instanceId = instanceId;
		candidate.moduleId = module.moduleId;
		for (int rotation = 0; rotation < 4; ++rotation)
		{
			for (int y = 0; y < rules.schematic.height; ++y)
			{
				for (int x = 0; x < rules.schematic.width; ++x)
				{
					candidate.origin = { x, y };
					candidate.rotationQuarterTurns = rotation;
					if (excludedOrigin && candidate.origin == *excludedOrigin) continue;
					if (vega::ModulePlacementRules::CanPlace(rules, loadout, candidate, ignoredInstanceId) ==
						vega::ModulePlacementFailure::None)
					{
						return std::make_pair(candidate.origin, rotation);
					}
				}
			}
		}
		return std::nullopt;
	}

	inline std::optional<vega::ModulePlacement> FindPlacementWithBestZoneBonus(
		const vega::ModuleRulesContext& rules,
		const vega::ModuleLoadout& loadout,
		const vega::ModuleDefinition& module,
		std::uint32_t instanceId
	)
	{
		vega::ModulePlacement candidate;
		candidate.instanceId = instanceId;
		candidate.moduleId = module.moduleId;
		std::optional<vega::ModulePlacement> bestPlacement;
		float bestBonus = -1.f;
		for (int rotation = 0; rotation < 4; ++rotation)
		{
			for (int y = 0; y < rules.schematic.height; ++y)
			{
				for (int x = 0; x < rules.schematic.width; ++x)
				{
					candidate.origin = { x, y };
					candidate.rotationQuarterTurns = rotation;
					if (vega::ModulePlacementRules::CanPlace(rules, loadout, candidate) != vega::ModulePlacementFailure::None) continue;
					const float bonus = vega::ModuleResolver::GetZoneBonus(rules, module, candidate);
					if (bonus > bestBonus)
					{
						bestBonus = bonus;
						bestPlacement = candidate;
					}
				}
			}
		}
		return bestPlacement;
	}

	inline std::optional<std::pair<sf::Vector2i, int>> FindPlacementWithCooldownZone(
		const vega::ModuleRulesContext& rules,
		const vega::ModuleLoadout& loadout,
		const vega::ModuleDefinition& module,
		std::uint32_t instanceId,
		const vega::GameplayTag& zone,
		std::uint32_t ignoredInstanceId = 0
	)
	{
		vega::ModulePlacement candidate;
		candidate.instanceId = instanceId;
		candidate.moduleId = module.moduleId;
		for (int rotation = 0; rotation < 4; ++rotation)
		{
			for (int y = 0; y < rules.schematic.height; ++y)
			{
				for (int x = 0; x < rules.schematic.width; ++x)
				{
					candidate.origin = { x, y };
					candidate.rotationQuarterTurns = rotation;
					if (vega::ModulePlacementRules::CanPlace(rules, loadout, candidate, ignoredInstanceId) !=
						vega::ModulePlacementFailure::None) continue;
					const vega::ModuleSizeVariant& variant = module.sizeVariants[static_cast<std::size_t>(candidate.variantIndex)];
					const vega::List<sf::Vector2i> cells = vega::ModuleShapeRules::Rotate(variant.shape.cells, rotation);
					const bool fitsZone = std::all_of(cells.begin(), cells.end(), [&rules, &candidate, &zone](const sf::Vector2i& cell)
					{
						return rules.schematic.GetZoneAt(candidate.origin.x + cell.x, candidate.origin.y + cell.y) == zone;
					});
					if (!fitsZone) continue;

					vega::List<vega::ModulePlacement> placements = loadout.GetPlacements();
					placements.erase(std::remove_if(placements.begin(), placements.end(),
						[instanceId](const vega::ModulePlacement& placement) { return placement.instanceId == instanceId; }), placements.end());
					placements.push_back(candidate);
					const auto effects = vega::ModuleResolver::Resolve(rules, placements);
					const bool resolvesZone = std::any_of(effects.cooldownRules.begin(), effects.cooldownRules.end(),
						[instanceId, &zone](const vega::ResolvedModuleCooldownRule& rule)
						{
							return rule.instanceId == instanceId && rule.zone == zone;
						});
					if (zone == vega::GameplayTag{ "Module.Zone.Nose" } ? !resolvesZone : resolvesZone)
						return std::make_pair(candidate.origin, rotation);
				}
			}
		}
		return std::nullopt;
	}

	inline std::uint32_t AddAndInstall(
		Report& report,
		vega::ModuleLoadout& loadout,
		const vega::ModuleRulesContext& rules,
		const char* moduleId
	)
	{
		const vega::ModuleDefinition* module = FindModule(rules, moduleId);
		report.Check(module != nullptr, std::string{ "catalog contains " } + moduleId);
		const std::uint32_t instanceId = loadout.AddToInventory(module->moduleId);
		const auto placement = FindPlacement(rules, loadout, *module, instanceId);
		report.Check(placement.has_value(), std::string{ "free schematic cell for " } + moduleId);
		report.Check(loadout.Install(rules, instanceId, placement->first, placement->second) ==
			vega::ModulePlacementFailure::None, std::string{ "install " } + moduleId);
		return instanceId;
	}

	inline vega::ModulePlacement FindInstalled(const vega::ModuleLoadout& loadout, std::uint32_t instanceId)
	{
		const vega::ModulePlacement* placement = loadout.FindPlacement(instanceId);
		if (!placement) throw std::runtime_error{ "installed module instance disappeared" };
		return *placement;
	}

	class RemoveModuleOnAttributeChange final
	{
	public:
		RemoveModuleOnAttributeChange(vega::ModuleLoadout& loadout, vega::ShipModuleApplier& applier,
			const vega::ModuleRulesContext& rules, std::uint32_t instanceId)
			: mLoadout{ loadout }, mApplier{ applier }, mRules{ rules }, mInstanceId{ instanceId }
		{
		}

		void OnAttributeChanged(sas::AttributeId id, float previousValue, float value)
		{
			if (mRemoved || id != vega::OwnerAttributeIds::EnergyPower || value <= previousValue) return;
			mRemoved = mLoadout.Uninstall(mInstanceId) && mApplier.Apply(mLoadout, mRules);
		}

		bool WasRemoved() const { return mRemoved; }

	private:
		vega::ModuleLoadout& mLoadout;
		vega::ShipModuleApplier& mApplier;
		const vega::ModuleRulesContext& mRules;
		std::uint32_t mInstanceId;
		bool mRemoved = false;
	};

	struct CapturedDamage
	{
		float originalDamage = 0.f;
		bool wasCritical = false;
		vega::List<vega::GameplayTag> damageTags;
		vega::List<vega::GameplayTag> sourceAbilityTags;
		bool skippedModuleRules = false;
	};

	class DamageCapture final
	{
	public:
		void OnDamage(const vega::DamageContext& context)
		{
			records.push_back({ context.originalDamage, context.wasCritical, context.damageTags,
				context.sourceAbilityTags, context.payload.skipModuleDamageRules });
		}

		vega::List<CapturedDamage> records;
	};

	class RemoveShatterCurrentOnPrimaryDamage final
	{
	public:
		RemoveShatterCurrentOnPrimaryDamage(vega::ModuleLoadout& loadout, vega::ShipModuleApplier& applier,
			const vega::ModuleRulesContext& rules, std::uint32_t instanceId)
			: mLoadout{ loadout }, mApplier{ applier }, mRules{ rules }, mInstanceId{ instanceId }
		{
		}

		void OnDamage(const vega::DamageContext& context)
		{
			if (mRemoved || context.payload.skipModuleDamageRules ||
				!std::any_of(context.sourceAbilityTags.begin(), context.sourceAbilityTags.end(),
					[](const vega::GameplayTag& tag) { return tag.MatchesTag(vega::GameplayTags::Ability::Primary); })) return;
			mRemoved = mLoadout.Uninstall(mInstanceId) && mApplier.Apply(mLoadout, mRules);
		}

		bool WasRemoved() const { return mRemoved; }

	private:
		vega::ModuleLoadout& mLoadout;
		vega::ShipModuleApplier& mApplier;
		const vega::ModuleRulesContext& mRules;
		std::uint32_t mInstanceId;
		bool mRemoved = false;
	};

	class DestroyTargetOnPrimaryDamage final
	{
	public:
		explicit DestroyTargetOnPrimaryDamage(vega::Actor& target) : mTarget{ target } {}

		void OnDamage(const vega::DamageContext& context)
		{
			if (!context.payload.skipModuleDamageRules && std::any_of(context.sourceAbilityTags.begin(), context.sourceAbilityTags.end(),
				[](const vega::GameplayTag& tag) { return tag.MatchesTag(vega::GameplayTags::Ability::Primary); })) mTarget.Destroy();
		}

	private:
		vega::Actor& mTarget;
	};


	void RunCommonModuleChecks(Report& report, vega::Application& application);
	void RunCooldownModuleChecks(Report& report, vega::Application& application);
	void RunFullMagazineChecks(Report& report, vega::Application& application);

	inline void WriteArtifact(const std::filesystem::path& path, const Json& artifact)
	{
		if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path());
		std::ofstream output{ path, std::ios::out | std::ios::trunc };
		if (!output) throw std::runtime_error{ "Could not open Common Module E2E artifact path" };
		output << artifact.dump(2) << '\n';
	}
}
