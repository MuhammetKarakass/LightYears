#include "UncommonModuleE2E.h"

int main(int argc, char** argv)
{
	using namespace uncommon_module_e2e;
	if (argc != 2)
	{
		std::cerr << "Usage: UncommonModuleE2ETests <artifact-path>\n";
		return 2;
	}

	Report report;
	const std::filesystem::path artifactPath{ argv[1] };
	const std::filesystem::path fixtureRoot = artifactPath.parent_path() / "uncommon-fixtures";
	try
	{
		ly::AssetManager::GetAssetManager().SetAssetRootDirectory(
			(std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/"
		);
		if (!ly::GameContentBootstrap::Register()) throw std::runtime_error{ "GameContentBootstrap::Register failed" };
		std::string windowTitle = "Uncommon module E2E";
		ly::Application application{ sf::Vector2u{ 64u, 64u }, 32u, windowTitle, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		RunUncommonContentChecks(report, fixtureRoot);
		RunUncommonRollerChecks(report);
		RunUncommonRuntimeChecks(report, application, fixtureRoot);
		report.artifact["test"] = "UncommonModuleE2E";
		report.artifact["passed"] = true;
	}
	catch (const std::exception& exception)
	{
		report.artifact["error"] = exception.what();
		std::cerr << "Uncommon module E2E failed: " << exception.what() << '\n';
	}

	try { common_module_e2e::WriteArtifact(artifactPath, report.artifact); }
	catch (const std::exception& exception)
	{
		std::cerr << "Could not write Uncommon Module E2E artifact: " << exception.what() << '\n';
		return 1;
	}
	std::cout << "E2E artifact: " << std::filesystem::absolute(artifactPath).string() << '\n';
	return report.artifact.value("passed", false) ? 0 : 1;
}
