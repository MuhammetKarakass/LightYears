#pragma once
#include "framework/Core.h"
#include <SFML/Graphics.hpp>
#include "framework/TimerManager.h"
#include "framework/FramePacer.h"
#include "framework/RenderViewport.h"
#include <utility>

namespace ly
{
	class World;
	class WorldRenderPipeline;
	class Application
	{
	public:

		Application(sf::Vector2u Position,unsigned int bit, const std::string& Title,uint32_t Style);
		virtual ~Application() = default;
		void Run();
		// Read after Run so tools can report failure while still using normal shutdown.
		virtual int GetExitCode() const { return 0; }

		void QuitApplication();

		sf::Vector2u GetWindowSize() const
		{
			return mWindow.getSize();
		}

		sf::RenderWindow& GetRenderWindow()
		{
			return mWindow;
		}

		const sf::RenderWindow& GetRenderWindow() const
		{
			return mWindow;
		}

		RenderViewport& GetWindowViewport() { return mWindowViewport; }
		const RenderViewport& GetWindowViewport() const { return mWindowViewport; }

		// Non-owning; the derived application owns the pipeline and must outlive the worlds that render with it.
		WorldRenderPipeline* GetWorldRenderPipeline() const { return mRenderPipeline; }
		void SetWorldRenderPipeline(WorldRenderPipeline* pipeline) { mRenderPipeline = pipeline; }

		template<typename WorldType, typename... Args>
		weak_ptr<WorldType> LoadWorld(Args&&... args);

		// The world that ticks and renders this frame; a LoadWorld request appears here from the next frame.
		World* GetCurrentWorld() const { return mCurrentWorld.get(); }

	protected:
		// Runs before the current world sees an event; returning true consumes it.
		virtual bool HandleApplicationEvent(const sf::Event& event);
		// Runs after the world and HUD were drawn and before the window is presented.
		virtual void AfterRender();

	private:
		sf::RenderWindow mWindow;
		WindowRenderViewport mWindowViewport;
		WorldRenderPipeline* mRenderPipeline = nullptr;
		float mTargetFrameRate;
		FramePacer mFramePacer;
		bool mShouldQuit;
		bool mQuitRequested;
		sf::Clock mTickClock;

		sf::Clock mCleanCycleClock;
		float mCleanCycleTime;

		shared_ptr<World> mCurrentWorld;
		shared_ptr<World> mPendingWorld;

		void TickInternal(float deltaTime);
		virtual void Tick(float deltaTime);

		void RenderInternal();
		virtual void Render(); 

		bool DispatchEvent(const std::optional<sf::Event>& event);
		void ShutdownApplication();


	};

	template<typename WorldType, typename... Args>
	weak_ptr<WorldType> Application::LoadWorld(Args&&... args)
	{

        auto newWorld = std::make_shared<WorldType>(this, std::forward<Args>(args)...);
        mPendingWorld = newWorld;
        return newWorld;
           
	}
}
