#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP4E2E.h"
#include "gameplay/MovementComponent.h"
#include "gameplay/ship/ShipRuntimeModifiers.h"
#include "gameplay/content/GameContentBootstrap.h"

using namespace uncommon_p4_e2e;

int main(int argc, char** argv)
{
	vega::InstallVegaEngineAssets();
	if (argc != 2) return 2;
	Report report;
	try
	{
		vega::EngineContext::Current().Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Slipstream content registration failed" };
		std::string title = "Slipstream Control E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		Fixture f{ report, application };
		f.ship->SetMovementMode(ShipMovementMode::ThrustDrift);
		const auto instance = f.Install("Module.Movement.EscapeVelocity.Rare", 1, 1, "Module.SideFeature.EscapeVelocity.SlipstreamControl");
		const auto turn = [&]() { return f.ship->GetRuntimeModifiers().GetTurnCapabilityMultiplier(); };
		report.CheckValue("before sonic there is no turn bonus", 1.f, turn(), "shipped Slipstream Control");
		for (int tick = 0; tick < 21; ++tick) { f.ship->SetVelocity({ 100000.f, 0.f }); f.level->TickInternal(0.1f); }
		report.CheckValue("sonic entry grants 25 percent turn capability", 1.25f, turn(), "two seconds at the live cap");
		// Unrelated layout changes must preserve a running modifier, not clear it during reconciliation.
		const auto filler = f.Install("Module.Utility.TargetingComputer.Basic");
		report.CheckValue("unrelated install preserves sonic turning", 1.25f, turn(), "modifier ownership across reapply");
		f.Remove(filler);
		report.CheckValue("unrelated removal preserves sonic turning", 1.25f, turn(), "same active rule survives");
		f.ship->SetVelocity({ 0.f, 0.f }); f.level->TickInternal(0.1f);
		report.CheckValue("slowing down clears Slipstream turning", 1.f, turn(), "no Afterglow on this placement");
		for (int tick = 0; tick < 21; ++tick) { f.ship->SetVelocity({ 100000.f, 0.f }); f.level->TickInternal(0.1f); }
		f.Remove(instance);
		report.CheckValue("uninstalling while sonic releases the turn modifier", 1.f, turn(), "RAII and removal path");
		report.artifact["passed"] = true;
	}
	catch (const std::exception& error) { report.artifact["error"] = error.what(); }
	report.artifact["test"] = "RareSlipstreamControlE2E";
	WriteArtifact(argv[1], report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
