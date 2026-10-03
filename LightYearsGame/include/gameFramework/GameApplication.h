#pragma once

#include <framework/Application.h>

#include <memory>
#include <string>

namespace ly
{
	class GameVisualPipeline;

	class GameApplication : public Application
	{
	public:
		GameApplication();
		~GameApplication() override;

	protected:
		bool HandleApplicationEvent(const sf::Event& event) override;
		void AfterRender() override;

	private:
		virtual void Tick(float deltaTime) override;

		// Dev-only command line: --visual-off, --visual-script <name>, --visual-capture <png> [--capture-frames N],
		// --visual-player-hull <0..1> (shown hull fraction only), --visual-ship <0..5> (start with that player ship),
		// --visual-assets <dir> (visual pipeline assets; default: <exe dir>/visual if present, else the source tree).
		struct DevOptions
		{
			bool visualOff = false;
			std::string colorScript = "nebula_teal_pink";
			std::string capturePath;
			int captureFrames = 180;
			float playerHull = -1.f;
			int shipIndex = -1;
			std::string visualAssets;
		};

		static DevOptions ParseDevOptions();
		void InitializeVisualPipeline();
		void SaveCaptureIfDue();

		DevOptions mDevOptions;
		std::unique_ptr<GameVisualPipeline> mVisualPipeline;
		int mRenderedFrames = 0;
	};
}
