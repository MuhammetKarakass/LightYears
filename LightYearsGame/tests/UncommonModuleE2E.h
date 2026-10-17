#pragma once

#include "CommonModuleE2ETestSupport.h"

namespace uncommon_module_e2e
{
	using Json = common_module_e2e::Json;
	using Report = common_module_e2e::Report;

	void RunUncommonContentChecks(Report& report, const std::filesystem::path& fixtureRoot);
	void RunUncommonRollerChecks(Report& report);
	void RunUncommonRuntimeChecks(Report& report, vega::Application& application, const std::filesystem::path& fixtureRoot);
	void RunRuntimeRuleFixtureChecks(Report& report, vega::Player& player, vega::PlayerSpaceShip& ship,
		common_module_e2e::HeadlessModuleLevel& level, const std::filesystem::path& fixtureRoot);
}
