#include "framework/Application.h"
#include "framework/AudioManager.h"
#include "framework/PhysicsSystem.h"
#include "framework/TimerManager.h"
#include "level/ArenaLevel.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <numeric>
#include <string>

namespace ly
{
	namespace
	{
		using Json = nlohmann::json;
		constexpr int SimulationFrameCount = 600;
		constexpr float SimulationDeltaSeconds = 1.f / 60.f;
		constexpr double TimerUpdateBudgetMilliseconds = 0.5;

		std::string& GetWindowTitle()
		{
			static std::string title{ "LightYears Timer Manager E2E" };
			return title;
		}

		class TimerMeasurementApplication final : public Application
		{
		public:
			TimerMeasurementApplication()
				: Application({ 320, 240 }, 32, GetWindowTitle(), sf::Style::None)
			{
				GetRenderWindow().setVisible(false);
			}
		};

		struct TimerSamples
		{
			std::vector<std::size_t> traversalCounts;
			std::vector<double> updateMicroseconds;
		};

		double Percentile(std::vector<double> samples, double percentile)
		{
			if (samples.empty()) return 0.0;
			std::sort(samples.begin(), samples.end());
			const std::size_t index = static_cast<std::size_t>(
				std::ceil(percentile * static_cast<double>(samples.size()))
			) - 1;
			return samples[std::min(index, samples.size() - 1)];
		}

		double Mean(const std::vector<double>& samples)
		{
			if (samples.empty()) return 0.0;
			return std::accumulate(samples.begin(), samples.end(), 0.0) /
				static_cast<double>(samples.size());
		}

		double MeanCount(const std::vector<std::size_t>& samples)
		{
			if (samples.empty()) return 0.0;
			const double total = std::accumulate(
				samples.begin(),
				samples.end(),
				0.0,
				[](double accumulated, std::size_t count) { return accumulated + static_cast<double>(count); }
			);
			return total / static_cast<double>(samples.size());
		}

		template<typename Update>
		double MeasureUpdate(Update&& update)
		{
			const auto start = std::chrono::steady_clock::now();
			update();
			const auto end = std::chrono::steady_clock::now();
			return std::chrono::duration<double, std::micro>{ end - start }.count();
		}

		Json SerializeSamples(const TimerSamples& samples)
		{
			std::size_t minimumCount = 0;
			std::size_t maximumCount = 0;
			if (!samples.traversalCounts.empty())
			{
				const auto counts = std::minmax_element(
					samples.traversalCounts.begin(), samples.traversalCounts.end()
				);
				minimumCount = *counts.first;
				maximumCount = *counts.second;
			}
			const double maximumUpdate = samples.updateMicroseconds.empty()
				? 0.0
				: *std::max_element(samples.updateMicroseconds.begin(), samples.updateMicroseconds.end());
			return {
				{ "countSamples", samples.traversalCounts.size() },
				{ "rawTraversalCounts", samples.traversalCounts },
				{ "rawUpdateMicroseconds", samples.updateMicroseconds },
				{ "traversalTimerCount", {
					{ "average", MeanCount(samples.traversalCounts) },
					{ "maximum", maximumCount },
					{ "minimum", minimumCount }
				} },
				{ "updateCostMicroseconds", {
					{ "average", Mean(samples.updateMicroseconds) },
					{ "maximum", maximumUpdate },
					{ "p50", Percentile(samples.updateMicroseconds, 0.50) },
					{ "p95", Percentile(samples.updateMicroseconds, 0.95) }
				} },
				{ "updateSamples", samples.updateMicroseconds.size() }
			};
		}

		bool WriteArtifact(const std::filesystem::path& path, const Json& artifact)
		{
			std::error_code error;
			if (!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path(), error);
			if (error) return false;
			std::ofstream output{ path, std::ios::out | std::ios::trunc };
			if (!output) return false;
			output << artifact.dump(2) << '\n';
			return output.good();
		}
	}

	int RunTimerManagerSceneE2E(const char* artifactPath, const std::string& setupError)
	{
		TimerManager::GetGlobalTimerManager().ClearAllTimers();
		TimerManager::GetGameTimerManager().ClearAllTimers();
		std::unique_ptr<TimerMeasurementApplication> application;
		std::shared_ptr<ArenaLevel> level;
		TimerSamples globalSamples;
		TimerSamples gameSamples;
		std::vector<double> combinedFrameUpdateMicroseconds;
		std::string runtimeError;
		bool levelStarted = false;
		bool physicsInitialized = false;
		int completedFrames = 0;

		if (setupError.empty())
		{
			try
			{
				application = std::make_unique<TimerMeasurementApplication>();
				level = std::make_shared<ArenaLevel>(application.get());
				PhysicsSystem::Get().InitializeWorld({ 0.f, 0.f });
				physicsInitialized = true;
				level->BeginPlayInternal();
				levelStarted = true;
				for (int frame = 0; frame < SimulationFrameCount; ++frame)
				{
					level->TickInternal(SimulationDeltaSeconds);
					const std::size_t globalCount = TimerManager::GetGlobalTimerManager().GetTraversalTimerCount();
					globalSamples.traversalCounts.push_back(globalCount);
					const double globalUpdate = MeasureUpdate([&]()
					{
						TimerManager::GetGlobalTimerManager().UpdateTimer(SimulationDeltaSeconds);
					});

					double gameUpdate = 0.0;
					const bool gameTimerActive = !level->IsPaused();
					if (gameTimerActive)
					{
						const std::size_t gameCount = TimerManager::GetGameTimerManager().GetTraversalTimerCount();
						gameSamples.traversalCounts.push_back(gameCount);
						gameUpdate = MeasureUpdate([&]()
						{
							TimerManager::GetGameTimerManager().UpdateTimer(SimulationDeltaSeconds);
						});
					}
					PhysicsSystem::Get().Step(SimulationDeltaSeconds);
					AudioManager::GetAudioManager().Update(SimulationDeltaSeconds);
					globalSamples.updateMicroseconds.push_back(globalUpdate);
					if (gameTimerActive) gameSamples.updateMicroseconds.push_back(gameUpdate);
					combinedFrameUpdateMicroseconds.push_back(globalUpdate + gameUpdate);
					++completedFrames;
				}
			}
			catch (const std::exception& exception)
			{
				runtimeError = exception.what();
			}
			catch (...)
			{
				runtimeError = "Unknown exception while ticking ArenaLevel";
			}
		}

		const double combinedP95Microseconds = Percentile(combinedFrameUpdateMicroseconds, 0.95);
		const double combinedMaximumMicroseconds = combinedFrameUpdateMicroseconds.empty()
			? 0.0
			: *std::max_element(combinedFrameUpdateMicroseconds.begin(), combinedFrameUpdateMicroseconds.end());
		const bool metricSamplesComplete = completedFrames == SimulationFrameCount &&
			globalSamples.traversalCounts.size() == SimulationFrameCount &&
			globalSamples.updateMicroseconds.size() == SimulationFrameCount &&
			gameSamples.traversalCounts.size() == SimulationFrameCount &&
			gameSamples.updateMicroseconds.size() == SimulationFrameCount;
		const bool measurementCompleted = setupError.empty() && runtimeError.empty() && levelStarted &&
			metricSamplesComplete;
		const bool schedulerGateExceeded =
			combinedP95Microseconds / 1000.0 > TimerUpdateBudgetMilliseconds;

		Json artifact{
			{ "assertions", {
				{ "bothApplicationTimerManagersMeasured", metricSamplesComplete },
				{ "realArenaLevelStartedAndTicked", levelStarted && completedFrames == SimulationFrameCount },
				{ "runtimeContentBootstrapSucceeded", setupError.empty() }
			} },
			{ "input", {
				{ "deltaSecondsPerFrame", SimulationDeltaSeconds },
				{ "frameCount", SimulationFrameCount },
				{ "sceneClass", "ArenaLevel" },
				{ "syntheticTimersAdded", 0 }
			} },
			{ "measurement", {
				{ "combinedFrameUpdateMicroseconds", combinedFrameUpdateMicroseconds },
				{ "combinedTimerUpdateCostMicroseconds", {
					{ "maximum", combinedMaximumMicroseconds },
					{ "p50", Percentile(combinedFrameUpdateMicroseconds, 0.50) },
					{ "p95", combinedP95Microseconds }
				} },
				{ "frameBudgetMilliseconds", 1000.0 / 60.0 },
				{ "schedulerGate", {
					{ "exceeded", schedulerGateExceeded },
					{ "p95ThresholdMilliseconds", TimerUpdateBudgetMilliseconds }
				} },
				{ "globalTimers", SerializeSamples(globalSamples) },
				{ "gameTimers", SerializeSamples(gameSamples) }
			} },
			{ "outcome", {
				{ "completedFrames", completedFrames },
				{ "levelStarted", levelStarted },
				{ "runtimeError", runtimeError },
				{ "schedulerChangeIndicated", schedulerGateExceeded },
				{ "setupError", setupError }
			} },
			{ "passed", measurementCompleted },
			{ "reproduction", {
				"cmake --build build --config Debug --target LightYearsGame LightYearsContinuousBeamWallE2ETests --parallel 4",
				"ctest --test-dir build -C Debug -R ^LightYearsTimerManagerSceneE2E$ --output-on-failure"
			} },
			{ "scenario", "d2.timer_manager_real_arena_scene_measurement" },
			{ "schemaVersion", 1 }
		};

		level.reset();
		TimerManager::GetGlobalTimerManager().ClearAllTimers();
		TimerManager::GetGameTimerManager().ClearAllTimers();
		if (physicsInitialized) PhysicsSystem::Get().Cleanup();
		application.reset();
		if (!WriteArtifact(std::filesystem::absolute(artifactPath), artifact))
		{
			std::cerr << "Could not write TimerManager scene E2E artifact.\n";
			return 1;
		}
		std::cout << "E2E artifact: " << std::filesystem::absolute(artifactPath).string() << '\n';
		std::cout << "ArenaLevel timer measurement: frames=" << completedFrames
			<< " globalCountMax=" << (globalSamples.traversalCounts.empty() ? 0 :
				*std::max_element(globalSamples.traversalCounts.begin(), globalSamples.traversalCounts.end()))
			<< " gameCountMax=" << (gameSamples.traversalCounts.empty() ? 0 :
				*std::max_element(gameSamples.traversalCounts.begin(), gameSamples.traversalCounts.end()))
			<< " combinedUpdateP95Us=" << combinedP95Microseconds << '\n';
		if (!measurementCompleted)
		{
			std::cerr << "TimerManager scene E2E measurement failed; see the artifact for outcomes.\n";
			return 1;
		}
		std::cout << "TimerManager scene E2E measurement completed.\n";
		return 0;
	}
}
