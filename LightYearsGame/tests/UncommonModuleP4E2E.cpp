#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP4E2E.h"

int main(int argc, char** argv)
{
	vega::InstallVegaEngineAssets();
	using namespace uncommon_p4_e2e;
	if (argc != 2) return 2;
	Report report;
	try
	{
		AssetManager::GetAssetManager().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "P4 content registration failed" };
		std::string title = "Uncommon P4 E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		DamageChecks(report, application);
		ReactionChecks(report, application);
		BridgeChecks(report, application);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& exception) { report.artifact["error"] = exception.what(); std::cerr << exception.what() << '\n'; }
	report.artifact["test"] = "UncommonModuleP4E2E";
	WriteArtifact(argv[1], report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
