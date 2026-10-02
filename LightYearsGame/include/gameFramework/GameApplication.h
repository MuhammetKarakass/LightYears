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

		// Dev-only command line: --visual-off, --visual-script <name>, --visual-capture <png> [--capture-frames N].
		struct DevOptions
		{
			bool visualOff = false;
			std::string colorScript = "nebula_teal_pink";
			std::string capturePath;
			int captureFrames = 180;
		};

		static DevOptions ParseDevOptions();
		void InitializeVisualPipeline();
		void SaveCaptureIfDue();

		DevOptions mDevOptions;
		std::unique_ptr<GameVisualPipeline> mVisualPipeline;
		int mRenderedFrames = 0;
	};
}
