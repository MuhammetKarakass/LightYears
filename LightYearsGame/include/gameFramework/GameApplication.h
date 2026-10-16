#pragma once

#include <framework/Application.h>
#include <framework/GameModule.h>

namespace ly
{
	class GameApplication : public Application
	{
	public:
		explicit GameApplication(bool editor = false);
		~GameApplication() override;
		int GetExitCode() const override { return mModule->ExitCode(); }
	protected:
		bool HandleApplicationEvent(const sf::Event& event) override { return mModule->HandleEvent(event); }
		void AfterRender() override { mModule->AfterRender(); }
	private:
		void Tick(float deltaTime) override { mModule->Tick(deltaTime); }
		std::unique_ptr<vega::GameModule> mModule;
	};
}
