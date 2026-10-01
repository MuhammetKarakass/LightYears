#include "framework/Application.h"
#include "framework/Core.h"
#include "framework/World.h"
#include "framework/AssetManager.h"
#include "framework/AudioManager.h"
#include "framework/PhysicsSystem.h"
#include "framework/TimerManager.h"
#include "framework/ShaderManager.h"
#include "framework/debug/Log.h"

#include <algorithm>
#include "framework/PerfMonitor.h"
#include <chrono>
#include <fstream>

namespace ly
{
	Application::Application(sf::Vector2u Position, unsigned int bit, std::string& Title, uint32_t Style)
		:mWindow{ sf::VideoMode(Position, bit), Title, Style },
		mTargetFrameRate{ 60.f },     
		mShouldQuit{ false },
		mQuitRequested{ false },
		mTickClock{},   
		mCurrentWorld{ nullptr },
		mCleanCycleClock{},    
		mCleanCycleTime{2.f}          
	{		
		mWindow.setFramerateLimit(60);
		mWindow.setVerticalSyncEnabled(false);
	}
	
	void Application::Run()
	{
		mTickClock.restart();
		// Hitch diagnostics: an empty file named disable_lights.txt next to the exe turns off additive light drawing for A/B testing.
		if (std::ifstream{ "disable_lights.txt" }.good())
		{
			ly::perf::g_disableLights.store(true);
			LY_CORE_INFO("Lights disabled by disable_lights.txt");
		}
		
		while (mWindow.isOpen() && !mShouldQuit)
		{
			LY_PROFILE_FRAME();
			sf::Time deltaTime = mTickClock.restart();
			float dt = deltaTime.asSeconds();
			
			while (const std::optional event = mWindow.pollEvent())
			{
				if (event->is<sf::Event::Closed>())
				{
					QuitApplication();
				}
				else 
				{
					DispatchEvent(event);
				}

				if (mQuitRequested)
				{
					break;
				}
			}

			if (mQuitRequested)
			{
				ShutdownApplication();
			}

			if (!mWindow.isOpen() || mShouldQuit)
			{
				break;
			}
			
			const auto hitchTickStart = std::chrono::steady_clock::now();
			TickInternal(dt);
			if (mQuitRequested)
			{
				ShutdownApplication();
				break;
			}

			const auto hitchRenderStart = std::chrono::steady_clock::now();
			RenderInternal();
			const auto hitchEnd = std::chrono::steady_clock::now();

			// Hitch diagnostics: report frames whose own work (not the frame-limiter
			// sleep) exceeded 25 ms, split into tick and render.
			const double hitchTickMs = std::chrono::duration<double, std::milli>(hitchRenderStart - hitchTickStart).count();
			const double hitchRenderMs = std::chrono::duration<double, std::milli>(hitchEnd - hitchRenderStart).count();
			// Frame statistics: sustained slowdowns (17-25 ms frames) never reach the hitch log, so aggregate every 5 s.
			static double statSeconds = 0.0, statTickMs = 0.0, statRenderMs = 0.0, statMaxFrameMs = 0.0;
			static int statFrames = 0, statOver20 = 0, statOver33 = 0;
			const double frameMs = dt * 1000.0;
			statSeconds += dt; statTickMs += hitchTickMs; statRenderMs += hitchRenderMs; ++statFrames;
			statMaxFrameMs = std::max(statMaxFrameMs, frameMs);
			if (frameMs > 20.0) ++statOver20;
			if (frameMs > 33.4) ++statOver33;
			if (statSeconds >= 5.0)
			{
				LY_CORE_INFO("FrameStats fps=%.1f avgTick=%.2fms avgRender=%.2fms maxFrame=%.1fms over20=%d over33=%d",
					statFrames / statSeconds, statTickMs / statFrames, statRenderMs / statFrames, statMaxFrameMs, statOver20, statOver33);
				statSeconds = statTickMs = statRenderMs = statMaxFrameMs = 0.0;
				statFrames = statOver20 = statOver33 = 0;
			}
			if (hitchTickMs > 25.0 || hitchRenderMs > 25.0)
			{
				LY_CORE_INFO("FrameHitch dt=%.1fms tick=%.1fms render=%.1fms actors=%d particles=%d bullets=%d", dt * 1000.f, hitchTickMs, hitchRenderMs,
				ly::perf::g_activeActors.load(), ly::perf::g_particles.load(), ly::perf::g_bullets.load());
			}
		}
	}

	void Application::QuitApplication()
	{
		mQuitRequested = true;
	}

	void Application::ShutdownApplication()
	{
		LY_PROFILE_FUNCTION();
		mPendingWorld.reset();
		mCurrentWorld.reset();
		AudioManager::ShutdownAudioManager();
		TimerManager::ShutdownTimerManagers();
		PhysicsSystem::ShutdownPhysicsSystem();
		ShaderManager::ShutdownShaderManager();
		AssetManager::ShutdownAssetManager();
		mWindow.close();
		mQuitRequested = false;
		mShouldQuit = true;
	}
	
	void Application::TickInternal(float deltaTime)
	{
		LY_PROFILE_FUNCTION();
		Tick(deltaTime);
		
		if(mCurrentWorld)
		{
			mCurrentWorld->TickInternal(deltaTime);
		}

		TimerManager::GetGlobalTimerManager().UpdateTimer(deltaTime);

		bool isPaused = mCurrentWorld && mCurrentWorld->IsPaused();

		if (!isPaused)
		{
			TimerManager::GetGameTimerManager().UpdateTimer(deltaTime);
			{
				LY_PROFILE_SCOPE("Physics.Step");
				PhysicsSystem::Get().Step(deltaTime);
			}
		}

		AudioManager::GetAudioManager().Update(deltaTime);
		
		if (mCleanCycleClock.getElapsedTime().asSeconds() > mCleanCycleTime)
		{
			mCleanCycleClock.restart();
			AssetManager::GetAssetManager().CleanCycle();
			AudioManager::GetAudioManager().CleanCycle();
			if (mCurrentWorld)
				mCurrentWorld->CleanCycle();
		}

		if(mPendingWorld && mPendingWorld!=mCurrentWorld)
		{
			mCurrentWorld = nullptr;

			TimerManager::GetGameTimerManager().ClearAllTimers();
			TimerManager::GetGlobalTimerManager().ClearAllTimers();

			PhysicsSystem::Get().Cleanup();
			PhysicsSystem::Get().InitializeWorld({ 0.f,0.f });

			mCurrentWorld = mPendingWorld;
			mCurrentWorld->BeginPlayInternal();
		}
	}
	
	void Application::Tick(float deltaTime)
	{

	}
	
	void Application::RenderInternal()
	{
		LY_PROFILE_FUNCTION();
		const auto clearStart = std::chrono::steady_clock::now();
		mWindow.clear();  
		const double clearMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - clearStart).count();
		if (clearMs > 20.0)
		{
			LY_CORE_INFO("ClearStall %.1fms", clearMs);
		}
		Render();         
		const auto displayStart = std::chrono::steady_clock::now();
		mWindow.display();
		const double displayMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - displayStart).count();
		// Hitch diagnostics: display() includes the 60 FPS limiter sleep (<= ~17 ms), so only larger values mean a GPU/driver/OS swap stall.
		if (displayMs > 30.0)
		{
			LY_CORE_INFO("DisplayStall %.1fms", displayMs);
		}
	}
	

	void Application::Render()
	{
		if(mCurrentWorld)
		{
			mCurrentWorld->Render(mWindow);
		}
		else
		{
			mWindow.clear(sf::Color::Black);
		}
	}
	
	bool Application::DispatchEvent(const std::optional<sf::Event>& event)
	{
		if (mCurrentWorld && event.has_value())
		{
			return mCurrentWorld->DispatchEvent(event.value());
		}
		return false;
	}
}
