#pragma once

#include <framework/Application.h>
#include <nlohmann/json.hpp>

#include <memory>
#include <string>

namespace ly
{
	class GameVisualPipeline;

	class GameApplication : public Application
	{
	public:
		explicit GameApplication(bool editor = false);
		~GameApplication() override;
		int GetExitCode() const override { return mExitCode; }

	protected:
		bool HandleApplicationEvent(const sf::Event& event) override;
		void AfterRender() override;
		std::string GetSceneBackdropState() const;
		void ApplySceneSettings(const nlohmann::json& settings);

	private:
		virtual void Tick(float deltaTime) override;

		// Dev-only command line: --visual-off, --visual-script <name>, --visual-capture <png> [--capture-frames N],
		// --visual-player-hull <0..1> (shown hull fraction only), --visual-ship <0..5> (start with that player ship),
		// --visual-assets <dir> (visual pipeline assets; default: <exe dir>/visual if present, else the source tree),
		// --galaxy-stop <index> (backdrop at that route stop, 0 = Sol), --warp <beta> (upward warp, clamped to warp.maxBeta).
		struct DevOptions
		{
			bool visualOff = false;
			std::string colorScript = "nebula_teal_pink";
			std::string capturePath;
			std::string scenePath;
			bool sceneSpecified = false;
			int captureFrames = 180;
			float playerHull = -1.f;
			int shipIndex = -1;
			std::string visualAssets;
			bool perfAutoPlay = false;
			int galaxyStop = 0;
			// Orbit speed steps from x1, as the + / - keys (3 = x8).
			int orbitSpeedSteps = 0;
			float warpBeta = 0.f;
		};

		static DevOptions ParseDevOptions();
		void InitializeVisualPipeline();
		void SaveCaptureIfDue();

		DevOptions mDevOptions;
		std::unique_ptr<GameVisualPipeline> mVisualPipeline;
		int mRenderedFrames = 0;
		int mExitCode = 0;
	};
}
