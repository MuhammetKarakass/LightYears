#pragma once

#include <framework/GameModule.h>

namespace ly
{
	class GameVisualPipeline;

	// Runtime services are shared by the standalone application and Play in editor.
	class LightYearsGameModule : public vega::GameModule
	{
	public:
		LightYearsGameModule();
		~LightYearsGameModule() override;
		const char* Name() const override { return "LightYears"; }
		void RegisterTypes() override;
		void ConfigureProject(const std::filesystem::path& file) override { mProjectFile = file; }
		void Initialize(Application& application, bool editor) override;
		std::shared_ptr<World> CreateLevel(Application& application, const nlohmann::json& scene, bool edit) override;
		std::shared_ptr<Actor> PlayerActor(World& world) override;
		vega::ActorDisplayInfo DisplayInfo(const Actor& actor) const override;
		void ApplySceneSettings(const nlohmann::json& settings) override;
		std::string SceneBackdropState() const override;
		void Tick(float deltaTime) override;
		bool HandleEvent(const sf::Event& event) override;
		void AfterRender() override;
		int ExitCode() const override { return mExitCode; }
	protected:
		GameVisualPipeline* VisualPipeline() const { return mVisualPipeline.get(); }
	private:
		struct DevOptions
		{
			bool visualOff = false;
			std::string colorScript = "nebula_teal_pink";
			std::string capturePath, scenePath;
			bool sceneSpecified = false;
			int captureFrames = 180;
			float playerHull = -1.f;
			int shipIndex = -1;
			std::string visualAssets;
			bool perfAutoPlay = false;
			int galaxyStop = 0, orbitSpeedSteps = 0;
			float warpBeta = 0.f;
		};
		static DevOptions ParseDevOptions();
		void InitializeVisualPipeline();
		void SaveCaptureIfDue();
		Application* mApplication = nullptr;
		std::filesystem::path mProjectFile;
		DevOptions mDevOptions;
		std::unique_ptr<GameVisualPipeline> mVisualPipeline;
		int mRenderedFrames = 0, mExitCode = 0;
	};

	sf::Vector2u GameProjectWindowSize(bool editor);
}
