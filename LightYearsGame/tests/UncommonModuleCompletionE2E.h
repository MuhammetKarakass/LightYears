#pragma once
#include "UncommonModuleP5E2E.h"
namespace uncommon_completion_e2e
{
	using namespace uncommon_p5_e2e;
	void DamageChecks(Report& report, Application& application);
	void ReactiveChecks(Report& report, Application& application);
	void ContentChecks(Report& report, const std::filesystem::path& artifactRoot);
}
