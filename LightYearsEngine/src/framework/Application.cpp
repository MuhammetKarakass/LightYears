#include "framework/Application.h"
#include "framework/Core.h"
#include "framework/World.h"
#include "framework/AssetManager.h"
#include "framework/AudioManager.h"
#include "framework/TimerManager.h"
#include "framework/ShaderManager.h"
#include "framework/debug/Log.h"

#include <algorithm>
#include "framework/PerfMonitor.h"
#include <chrono>
#include <fstream>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace ly
{
	namespace
	{
		// Upper bound for simulated time per frame so a long hitch cannot cause a huge simulation step.
		constexpr float MaxSimulationDeltaSeconds = 0.1f;
	}

	Application::Application(sf::Vector2u Position, unsigned int bit, const std::string& Title, uint32_t Style)
		:mWindow{ sf::VideoMode(Position, bit), Title, Style },
		mTargetFrameRate{ 60.f },     
		mFramePacer{ mTargetFrameRate },
		mShouldQuit{ false },
		mQuitRequested{ false },
		mTickClock{},   
		mCurrentWorld{ nullptr },
		mCleanCycleClock{},    
		mCleanCycleTime{2.f}          
	{		
		// The frame rate is held by mFramePacer; SFML's own limiter drifts (see FramePacer.h).
		mWindow.setVerticalSyncEnabled(false);
#ifdef _WIN32
		// Background work at normal priority (indexers, builds, browsers) otherwise takes the cores from the game
		// thread and the OpenGL driver thread in the middle of a frame, which showed up as 50-300 ms frames.
		SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);
#endif
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
			TickInternal(std::min(dt, MaxSimulationDeltaSeconds));
			if (mQuitRequested)
			{
				ShutdownApplication();
				break;
			}

			const auto hitchRenderStart = std::chrono::steady_clock::now();
			RenderInternal();
			const auto hitchEnd = std::chrono::steady_clock::now();
			mFramePacer.Wait();

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
			static std::vector<debug::FrameEvent> frameEvents;
			debug::Profiler::TakeFrameEvents(frameEvents);
			if (hitchTickMs > 25.0 || hitchRenderMs > 25.0)
			{
				LY_CORE_INFO("FrameHitch dt=%.1fms tick=%.1fms render=%.1fms actors=%d particles=%d bullets=%d", dt * 1000.f, hitchTickMs, hitchRenderMs,
				ly::perf::g_activeActors.load(), ly::perf::g_particles.load(), ly::perf::g_bullets.load());
				// The slow scopes and actors of this frame; nested scopes each report their own total.
				std::sort(frameEvents.begin(), frameEvents.end(), [](const debug::FrameEvent& a, const debug::FrameEvent& b) { return a.microseconds > b.microseconds; });
				for (std::size_t i = 0; i < frameEvents.size() && i < 10; ++i)
				{
					LY_CORE_INFO("  HitchPart %s %.1fms", frameEvents[i].name, frameEvents[i].microseconds / 1000.0);
				}
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
		ShaderManager::ShutdownShaderManager();
		AssetManager::ShutdownAssetManager();
		mWindow.close();
		mQuitRequested = false;
		mShouldQuit = true;
	}
	
	void Application::TickInternal(float deltaTime)
	{
		LY_PROFILE_FUNCTION();
		{
			LY_PROFILE_SCOPE("App.Tick");
			Tick(deltaTime);
		}
		
		if(mCurrentWorld)
		{
			mCurrentWorld->TickInternal(deltaTime);
		}

		{
			LY_PROFILE_SCOPE("App.GlobalTimers");
			TimerManager::GetGlobalTimerManager().UpdateTimer(deltaTime);
		}

		bool isPaused = mCurrentWorld && mCurrentWorld->IsPaused();

		if (!isPaused)
		{
			{
				LY_PROFILE_SCOPE("App.GameTimers");
				TimerManager::GetGameTimerManager().UpdateTimer(deltaTime);
			}
			if (mCurrentWorld)
			{
				mCurrentWorld->StepPhysics(deltaTime);
			}
		}

		{
			LY_PROFILE_SCOPE("App.Audio");
			AudioManager::GetAudioManager().Update(deltaTime);
		}
		
		if (mCleanCycleClock.getElapsedTime().asSeconds() > mCleanCycleTime)
		{
			LY_PROFILE_SCOPE("App.CleanCycle");
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
		AfterRender();
		const auto displayStart = std::chrono::steady_clock::now();
		mWindow.display();
		const double displayMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - displayStart).count();
		// Hitch diagnostics: display() no longer contains the frame limiter sleep, so a long swap means a GPU/driver/OS stall.
		if (displayMs > 20.0)
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
	
	bool Application::HandleApplicationEvent(const sf::Event&)
	{
		return false;
	}

	void Application::AfterRender()
	{
	}

	bool Application::DispatchEvent(const std::optional<sf::Event>& event)
	{
		if (event.has_value() && HandleApplicationEvent(event.value()))
		{
			return true;
		}
		if (mCurrentWorld && event.has_value())
		{
			return mCurrentWorld->DispatchEvent(event.value());
		}
		return false;
	}
}
