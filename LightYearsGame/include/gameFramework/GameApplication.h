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
		int GetExitCode() const override
		{
			const auto* hooks = vega::QueryCapability<vega::IAppLoopHooks>(mModule.get());
			return hooks ? hooks->ExitCode() : 0;
		}
	protected:
		bool HandleApplicationEvent(const sf::Event& event) override
		{
			auto* hooks = vega::QueryCapability<vega::IAppLoopHooks>(mModule.get());
			return hooks && hooks->HandleEvent(event);
		}
		void AfterRender() override
		{
			if (auto* hooks = vega::QueryCapability<vega::IAppLoopHooks>(mModule.get())) hooks->AfterRender();
		}
	private:
		void Tick(float deltaTime) override
		{
			if (auto* hooks = vega::QueryCapability<vega::IAppLoopHooks>(mModule.get())) hooks->Tick(deltaTime);
		}
		std::unique_ptr<vega::GameModule> mModule;
	};
}
