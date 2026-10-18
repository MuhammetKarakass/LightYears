#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP5E2E.h"

int main(int argc, char** argv)
{
	vega::InstallVegaEngineAssets();
	using namespace uncommon_p5_e2e;
	if (argc != 2) return 2;
	Report report;
	try
	{
		vega::EngineContext::Current().Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "P5 content registration failed" };
		std::string title = "Uncommon P5 E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		StatusChecks(report, application);
		ContentChecks(report, std::filesystem::path{ argv[1] }.parent_path() / "p5-fixtures");
		report.artifact["passed"] = true;
	}
	catch (const std::exception& exception) { report.artifact["error"] = exception.what(); std::cerr << exception.what() << '\n'; }
	report.artifact["test"] = "UncommonModuleP5E2E";
	WriteArtifact(argv[1], report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
