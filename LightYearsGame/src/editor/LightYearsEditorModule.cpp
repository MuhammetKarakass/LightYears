#include "editor/LightYearsEditorModule.h"
#include "gameFramework/LightYearsGameModule.h"
#include "player/PlayerManager.h"
#include "player/PlayerSpaceShip.h"
#include <editor/EditorExtensions.h>
#include <editor/EditorCommands.h>
#include <editor/EditorSkyEditor.h>
#include <gas/GasEditorExtension.h>
#include <presentation/visual/GameVisualPipeline.h>
#include <presentation/visual/SkyMapping.h>
#include <framework/scene/SceneSettings.h>

#include <algorithm>

namespace ly
{
	namespace
	{
		// Route IDs happen to equal the legacy collection IDs. Keep that mapping
		// game-side: the Sky module receives only opaque asset IDs and previews.
		class LightYearsSkyPreview final : public vega::sky::SkyEditorHost
		{
		public:
			explicit LightYearsSkyPreview(GameVisualPipeline& pipeline) : mPipeline{pipeline}
			{
				for (const auto& stop : pipeline.Route().Stops()) mSkies.push_back({SkyIdForRouteStop(stop.id), stop.name});
			}
			vega::sky::VisualAssets& EditorAssets() override { return mPipeline.EditorAssets(); }
			const std::vector<vega::sky::SkyPreview>& Skies() const override { return mSkies; }
			void SelectSky(std::size_t index) override { mPipeline.SetRouteStopIndex(index); }
			std::string SelectedSky() const override { return SkyIdForRouteStop(mPipeline.BackdropStopId()); }
			std::size_t SceneSelection(const vega::EditorContext& context) const override
			{
				const auto value = context.sceneSettings ? SceneSettingsSchema::Get().Value(*context.sceneSettings, "routeStop") : nlohmann::json(0);
				const auto index = value.is_number_integer() ? value.get<int>() : 0;
				return mSkies.empty() ? 0 : std::min(static_cast<std::size_t>(std::max(index, 0)), mSkies.size() - 1);
			}
			void UseInScene(vega::CommandHistory& history, std::size_t index) override
			{
				history.Execute(std::make_unique<vega::SetSceneSettingCommand>("routeStop", static_cast<int>(index)));
			}
			void SetSkyAuthoring(bool enabled) override { mPipeline.SetSkyAuthoring(enabled); }
			vega::sky::SkyGeometry Geometry(sf::Vector2u output) const override
			{
				const auto geometry = mPipeline.EditorBackdrop().EditorGeometry(mPipeline.EditorAssets(), output);
				return {geometry.planetAt, geometry.bandDrift, geometry.planetRadius, geometry.planetBaseScale, geometry.band, geometry.mirrored, geometry.hasPlanet};
			}
			vega::sky::SkyPalette Palette() const override { return vega::sky::SkyPaletteFor(mPipeline.EditorBackdrop().Sky()); }
			bool RenderSky(sf::RenderTexture& target, std::size_t index, const World& world, sf::Vector2u size) override
			{
				if (index >= mSkies.size()) return false;
				if (!mPreview) mPreview = std::make_unique<vega::sky::VisualPipeline>(EditorAssets());
				if (target.getSize() != size && !target.resize(size)) return false;
				if (mPreview->OutputSize() != size && !mPreview->Resize(size)) return false;
				const auto& stop = mPipeline.Route().Stops()[index];
				if (mRendered != stop.id) { mBackdrop.SetRouteStop(stop); mRendered = stop.id; }
				mPreview->BeginFrame();
				mBackdrop.Draw(mPipeline.EditorAssets(), *mPreview, world, sf::View{{960.f, 540.f}, {1920.f, 1080.f}}, 0.f);
				target.clear(); mPreview->Compose(target); target.display();
				return true;
			}
		private:
			GameVisualPipeline& mPipeline;
			std::vector<vega::sky::SkyPreview> mSkies;
			std::unique_ptr<vega::sky::VisualPipeline> mPreview;
			ProceduralBackdrop mBackdrop;
			std::string mRendered;
		};

		class LightYearsEditorModule final : public LightYearsGameModule
		{
		public:
			void RegisterEditorExtensions(vega::EditorExtensions& extensions) override
			{
				sas::RegisterEditorExtension(extensions);
				if (auto* pipeline = VisualPipeline())
				{
					auto sky = std::make_shared<vega::EditorSkyEditor>(std::make_shared<LightYearsSkyPreview>(*pipeline));
					sky->Register(extensions, sky);
				}
				extensions.AddScriptCommand("expect-player-ship", [](vega::EditorContext& context, vega::CommandHistory&,
					const std::vector<std::string>& arguments, std::string& detail)
				{
					if (arguments.size() != 1 || arguments[0].empty()) { detail = "Invalid arguments"; return false; }
					if (const auto* player = PlayerManager::GetPlayerManager().GetPlayer())
						if (const auto ship = player->GetCurrentSpaceShip().lock())
						{
							detail = "Spawned player ship = " + player->GetShipId();
							return context.playMode && ship->GetWorld() == context.world && player->GetShipId() == arguments[0];
						}
					return false;
				});
			}
		};
	}

	std::unique_ptr<vega::GameModule> CreateLightYearsEditorModule()
	{
		return std::make_unique<LightYearsEditorModule>();
	}
}
