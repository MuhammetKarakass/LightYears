#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleCompletionE2E.h"
int main(int argc, char** argv)
{
	ly::InstallLightYearsEngineAssets();
	using namespace uncommon_completion_e2e;
	if (argc != 2) return 2;
	Report report;
	report.artifact["input"] = { { "levels", { 1, 2, 3 } }, { "equipRemoveCyclesPerSide", 10 }, { "GoldPerSidePerKill", 1 }, { "O4BonusSeconds", 1 }, { "O5FirstHitMultiplier", 1.15 }, { "GlassHealthThreshold", .5 }, { "GlassCriticalChancePoints", 10 }, { "GritHealthThreshold", .35 }, { "GritShieldRegenMultiplier", 2 }, { "MovementFadeSeconds", 1.5 } };
	try
	{
		AssetManager::GetAssetManager().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Completion content registration failed" };
		std::string title = "Uncommon completion E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		uncommon_completion_e2e::DamageChecks(report, application);
		uncommon_completion_e2e::ReactiveChecks(report, application);
		uncommon_completion_e2e::ContentChecks(report, std::filesystem::path{ argv[1] }.parent_path() / "completion-fixtures");
		report.artifact["passed"] = true;
	}
	catch (const std::exception& exception) { report.artifact["error"] = exception.what(); std::cerr << exception.what() << '\n'; }
	report.artifact["test"] = "UncommonModuleCompletionE2E";
	WriteArtifact(argv[1], report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
