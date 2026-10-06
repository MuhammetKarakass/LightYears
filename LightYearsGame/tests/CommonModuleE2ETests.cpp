#include "CommonModuleE2ETestSupport.h"

int main(int argc, char** argv)
{
	using namespace common_module_e2e;
	if (argc != 2)
	{
		std::cerr << "Usage: CommonModuleE2ETests <artifact-path>\n";
		return 2;
	}

	Report report;
	const std::filesystem::path artifactPath{ argv[1] };
	try
	{
		ly::AssetManager::GetAssetManager().SetAssetRootDirectory(
			(std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/"
		);
		if (!ly::GameContentBootstrap::Register()) throw std::runtime_error{ "GameContentBootstrap::Register failed" };
		std::string windowTitle = "Common modules E2E";
		ly::Application application{ sf::Vector2u{ 64u, 64u }, 32u, windowTitle, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		RunCommonModuleChecks(report, application);
		RunCooldownModuleChecks(report, application);
		RunFullMagazineChecks(report, application);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& exception)
	{
		report.artifact["error"] = exception.what();
		std::cerr << "Common module E2E failed: " << exception.what() << '\n';
	}

	try { WriteArtifact(artifactPath, report.artifact); }
	catch (const std::exception& exception)
	{
		std::cerr << "Could not write Common Module E2E artifact: " << exception.what() << '\n';
		return 1;
	}
	std::cout << "E2E artifact: " << std::filesystem::absolute(artifactPath).string() << '\n';
	return report.artifact.value("passed", false) ? 0 : 1;
}
