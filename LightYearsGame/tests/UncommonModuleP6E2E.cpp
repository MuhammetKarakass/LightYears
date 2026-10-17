#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP6E2E.h"

int main(int argc, char** argv)
{
	vega::InstallVegaEngineAssets();
	using namespace uncommon_p6_e2e;
	if (argc != 2 && argc != 3) return 2;
	const bool ricochetOnly = argc == 3 && std::string{ argv[2] } == "ricochet";
	const bool balanceOnly = argc == 3 && std::string{ argv[2] } == "balance";
	const bool revisionOnly = argc == 3 && std::string{ argv[2] } == "revision";
	Report report;
	try
	{
		AssetManager::GetAssetManager().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "P6 content registration failed" };
		std::string title = "Uncommon P6 E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		if (!revisionOnly) RicochetChecks(report, application);
		if (balanceOnly) BalanceChecks(report, application);
		if (!ricochetOnly && !revisionOnly) ActorChecks(report, application);
		if (revisionOnly) { SizingChecks(report, application); DeathBurstChecks(report, application); }
		if (!ricochetOnly && !balanceOnly) MineChecks(report, application);
		uncommon_p6_e2e::ContentChecks(report, std::filesystem::path{ argv[1] }.parent_path() / "p6-fixtures");
		report.artifact["passed"] = true;
	}
	catch (const std::exception& exception) { report.artifact["error"] = exception.what(); std::cerr << exception.what() << '\n'; }
	report.artifact["test"] = "UncommonModuleP6E2E";
	report.artifact["scope"] = revisionOnly ? "Module sizing, AP-scaled mines, Thermal death burst and loader contracts" : (balanceOnly ? "Primary balance, incoming triggers, movement steps, ricochet and Scatter" : (ricochetOnly ? "Ricochet and generated-content validation" : "Full P6"));
	WriteArtifact(argv[1], report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
