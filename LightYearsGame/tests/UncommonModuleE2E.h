#pragma once

#include "CommonModuleE2ETestSupport.h"

namespace uncommon_module_e2e
{
	using Json = common_module_e2e::Json;
	using Report = common_module_e2e::Report;

	void RunUncommonContentChecks(Report& report, const std::filesystem::path& fixtureRoot);
	void RunUncommonRollerChecks(Report& report);
	void RunUncommonRuntimeChecks(Report& report, ly::Application& application, const std::filesystem::path& fixtureRoot);
	void RunRuntimeRuleFixtureChecks(Report& report, ly::Player& player, ly::PlayerSpaceShip& ship,
		common_module_e2e::HeadlessModuleLevel& level, const std::filesystem::path& fixtureRoot);
}
