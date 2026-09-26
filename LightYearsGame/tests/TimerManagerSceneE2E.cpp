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
#include <stdexcept>
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

		Json RunOneShotThrowScenario(TimerManager& timerManager, const std::weak_ptr<Object>& listener)
		{
			constexpr const char* expectedErrorId = "E16.one-shot-callback";
			const std::size_t timerCountBeforeClear = timerManager.GetTraversalTimerCount();
			timerManager.ClearAllTimers();
			const std::size_t timerCountAfterClear = timerManager.GetTraversalTimerCount();
			int callbackCount = 0;
			std::vector<std::string> callbackOrder;
			std::string firstUpdateErrorId;
			std::string secondUpdateErrorId;
			const TimerHandle oneShotHandle = timerManager.SetTimer(listener, [&]()
			{
				++callbackCount;
				callbackOrder.emplace_back("one-shot-callback");
				throw std::runtime_error{ expectedErrorId };
			}, SimulationDeltaSeconds, false);
			const std::size_t timerCountBeforeFirstUpdate = timerManager.GetTraversalTimerCount();
			try
			{
				timerManager.UpdateTimer(SimulationDeltaSeconds);
			}
			catch (const std::exception& exception)
			{
				firstUpdateErrorId = exception.what();
			}
			catch (...)
			{
				firstUpdateErrorId = "unknown-exception";
			}
			const std::size_t timerCountAfterFirstUpdate = timerManager.GetTraversalTimerCount();
			try
			{
				timerManager.UpdateTimer(SimulationDeltaSeconds);
			}
			catch (const std::exception& exception)
			{
				secondUpdateErrorId = exception.what();
			}
			catch (...)
			{
				secondUpdateErrorId = "unknown-exception";
			}
			const std::size_t timerCountAfterSecondUpdate = timerManager.GetTraversalTimerCount();
			timerManager.ClearAllTimers();
			const std::size_t timerCountAfterCleanup = timerManager.GetTraversalTimerCount();
			const bool firstErrorPreserved = firstUpdateErrorId == expectedErrorId;
			const bool oneShotNotRetried = callbackCount == 1 && secondUpdateErrorId.empty();
			const bool timerConsumed = timerCountAfterFirstUpdate == 0 && timerCountAfterSecondUpdate == 0;
			return {
				{ "assertions", {
					{ "firstErrorPreserved", firstErrorPreserved },
					{ "oneShotConsumedAfterThrow", timerConsumed },
					{ "oneShotNotRetried", oneShotNotRetried }
				} },
				{ "callbackOrder", callbackOrder },
				{ "caseId", "E16" },
				{ "counts", {
					{ "callbackInvocations", callbackCount },
					{ "timersAfterCleanup", timerCountAfterCleanup },
					{ "timersAfterFirstUpdate", timerCountAfterFirstUpdate },
					{ "timersAfterSecondUpdate", timerCountAfterSecondUpdate },
					{ "timersBeforeClear", timerCountBeforeClear },
					{ "timersBeforeFirstUpdate", timerCountBeforeFirstUpdate },
					{ "timersAfterClear", timerCountAfterClear }
				} },
				{ "errors", {
					{ "expectedFirstUpdateErrorId", expectedErrorId },
					{ "firstUpdateErrorId", firstUpdateErrorId },
					{ "secondUpdateErrorId", secondUpdateErrorId }
				} },
				{ "expected", {
					{ "callbackInvocations", 1 },
					{ "firstUpdateErrorId", expectedErrorId },
					{ "secondUpdateErrorId", "" },
					{ "timersAfterFirstUpdate", 0 },
					{ "timersAfterSecondUpdate", 0 }
				} },
				{ "actual", {
					{ "callbackInvocations", callbackCount },
					{ "firstUpdateErrorId", firstUpdateErrorId },
					{ "secondUpdateErrorId", secondUpdateErrorId },
					{ "timersAfterFirstUpdate", timerCountAfterFirstUpdate },
					{ "timersAfterSecondUpdate", timerCountAfterSecondUpdate }
				} },
				{ "handles", { { "oneShot", oneShotHandle.GetTimerKey() } } },
				{ "input", {
					{ "durationSeconds", SimulationDeltaSeconds },
					{ "firstUpdateDeltaSeconds", SimulationDeltaSeconds },
					{ "repeat", false },
					{ "secondUpdateDeltaSeconds", SimulationDeltaSeconds }
				} },
				{ "passed", timerCountBeforeFirstUpdate == 1 && timerCountAfterCleanup == 0 &&
					firstErrorPreserved && oneShotNotRetried && timerConsumed },
				{ "scenario", "luna.e16.one_shot_throw_and_second_update" },
				{ "schemaVersion", 1 }
			};
		}

		Json RunClearAddNestedUpdateScenario(TimerManager& timerManager, const std::weak_ptr<Object>& listener)
		{
			constexpr float outerUpdateDeltaSeconds = 0.125f;
			constexpr float nestedUpdateDeltaSeconds = 1.f;
			constexpr float followUpTimerDurationSeconds = 0.25f;
			const std::size_t timerCountBeforeClear = timerManager.GetTraversalTimerCount();
			timerManager.ClearAllTimers();
			const std::size_t timerCountAfterClear = timerManager.GetTraversalTimerCount();
			int outerCallbackCount = 0;
			int followUpCallbackCount = 0;
			std::vector<std::string> callbackOrder;
			std::string nestedUpdateError;
			std::string outerUpdateError;
			std::string firstFollowUpUpdateError;
			std::string secondFollowUpUpdateError;
			std::string listenerLifetimeUpdateError;
			std::string pendingListenerUpdateError;
			unsigned int followUpHandleKey = 0;
			const TimerHandle outerHandle = timerManager.SetTimer(listener, [&]()
			{
				++outerCallbackCount;
				callbackOrder.emplace_back("outer-callback-enter");
				timerManager.ClearAllTimers();
				callbackOrder.emplace_back("clear-all");
				const TimerHandle followUpHandle = timerManager.SetTimer(listener, [&]()
				{
					++followUpCallbackCount;
					callbackOrder.emplace_back("follow-up-callback");
				}, followUpTimerDurationSeconds, false);
				followUpHandleKey = followUpHandle.GetTimerKey();
				callbackOrder.emplace_back("follow-up-registered");
				callbackOrder.emplace_back("nested-update-requested");
				try
				{
					timerManager.UpdateTimer(nestedUpdateDeltaSeconds);
				}
				catch (const std::exception& exception)
				{
					nestedUpdateError = exception.what();
				}
				catch (...)
				{
					nestedUpdateError = "unknown-exception";
				}
				callbackOrder.emplace_back("nested-update-returned");
				callbackOrder.emplace_back("outer-callback-return");
			}, outerUpdateDeltaSeconds, false);
			const std::size_t timerCountBeforeOuterUpdate = timerManager.GetTraversalTimerCount();
			try
			{
				timerManager.UpdateTimer(outerUpdateDeltaSeconds);
			}
			catch (const std::exception& exception)
			{
				outerUpdateError = exception.what();
			}
			catch (...)
			{
				outerUpdateError = "unknown-exception";
			}
			callbackOrder.emplace_back("outer-update-returned");
			const std::size_t timerCountAfterOuterUpdate = timerManager.GetTraversalTimerCount();
			const int followUpCallbackCountAfterOuterUpdate = followUpCallbackCount;
			try
			{
				timerManager.UpdateTimer(outerUpdateDeltaSeconds);
			}
			catch (const std::exception& exception)
			{
				firstFollowUpUpdateError = exception.what();
			}
			catch (...)
			{
				firstFollowUpUpdateError = "unknown-exception";
			}
			callbackOrder.emplace_back("follow-up-update-1-returned");
			const int followUpCallbackCountAfterFirstUpdate = followUpCallbackCount;
			const std::size_t timerCountAfterFirstFollowUpUpdate = timerManager.GetTraversalTimerCount();
			try
			{
				timerManager.UpdateTimer(outerUpdateDeltaSeconds);
			}
			catch (const std::exception& exception)
			{
				secondFollowUpUpdateError = exception.what();
			}
			catch (...)
			{
				secondFollowUpUpdateError = "unknown-exception";
			}
			callbackOrder.emplace_back("follow-up-update-2-returned");
			const std::size_t timerCountAfterSecondFollowUpUpdate = timerManager.GetTraversalTimerCount();

			std::shared_ptr<Object> lifetimeOwner = std::make_shared<Object>();
			const unsigned int lifetimeOwnerId = lifetimeOwner->GetUniqueID();
			const std::weak_ptr<Object> weakLifetimeOwner{ lifetimeOwner };
			int lifetimeCallbackCount = 0;
			bool listenerAliveInsideCallback = false;
			const TimerHandle lifetimeHandle = timerManager.SetTimer(weakLifetimeOwner, [&]()
			{
				++lifetimeCallbackCount;
				lifetimeOwner.reset();
				listenerAliveInsideCallback = !weakLifetimeOwner.expired();
				callbackOrder.emplace_back("listener-lifetime-callback");
			}, 0.f, false);
			const std::size_t timerCountBeforeLifetimeUpdate = timerManager.GetTraversalTimerCount();
			try
			{
				timerManager.UpdateTimer(0.f);
			}
			catch (const std::exception& exception)
			{
				listenerLifetimeUpdateError = exception.what();
			}
			catch (...)
			{
				listenerLifetimeUpdateError = "unknown-exception";
			}
			const bool listenerExpiredAfterCallback = weakLifetimeOwner.expired();
			const std::size_t timerCountAfterLifetimeUpdate = timerManager.GetTraversalTimerCount();

			std::shared_ptr<Object> pendingOwner = std::make_shared<Object>();
			const unsigned int pendingOwnerId = pendingOwner->GetUniqueID();
			pendingOwner->Destroy();
			int pendingListenerCallbackCount = 0;
			const TimerHandle pendingListenerHandle = timerManager.SetTimer(pendingOwner, [&]()
			{
				++pendingListenerCallbackCount;
			}, 0.f, false);
			const std::size_t timerCountBeforePendingListenerUpdate = timerManager.GetTraversalTimerCount();
			try
			{
				timerManager.UpdateTimer(0.f);
			}
			catch (const std::exception& exception)
			{
				pendingListenerUpdateError = exception.what();
			}
			catch (...)
			{
				pendingListenerUpdateError = "unknown-exception";
			}
			const std::size_t timerCountAfterPendingListenerUpdate = timerManager.GetTraversalTimerCount();
			timerManager.ClearAllTimers();
			const std::size_t timerCountAfterCleanup = timerManager.GetTraversalTimerCount();
			const std::vector<std::string> expectedCallbackOrder{
				"outer-callback-enter", "clear-all", "follow-up-registered", "nested-update-requested",
				"nested-update-returned", "outer-callback-return", "outer-update-returned",
				"follow-up-update-1-returned", "follow-up-callback", "follow-up-update-2-returned",
				"listener-lifetime-callback"
			};
			const bool noUnexpectedExceptions = nestedUpdateError.empty() && outerUpdateError.empty() &&
				firstFollowUpUpdateError.empty() && secondFollowUpUpdateError.empty() &&
				listenerLifetimeUpdateError.empty() && pendingListenerUpdateError.empty();
			const bool followUpFiredAtCorrectTime = followUpCallbackCountAfterOuterUpdate == 0 &&
				followUpCallbackCountAfterFirstUpdate == 0 &&
				followUpCallbackCount == 1 && timerCountAfterFirstFollowUpUpdate == 1 &&
				timerCountAfterSecondFollowUpUpdate == 0;
			const bool timerCountsCorrect = timerCountAfterClear == 0 && timerCountBeforeOuterUpdate == 1 &&
				timerCountAfterOuterUpdate == 1 && timerCountBeforeLifetimeUpdate == 1 &&
				timerCountAfterLifetimeUpdate == 0 && timerCountBeforePendingListenerUpdate == 1 &&
				timerCountAfterPendingListenerUpdate == 0 && timerCountAfterCleanup == 0;
			const bool listenerLifetimeCorrect = lifetimeCallbackCount == 1 && listenerAliveInsideCallback &&
				listenerExpiredAfterCallback;
			const bool pendingListenerRejected = pendingListenerCallbackCount == 0;
			return {
				{ "assertions", {
					{ "clearAddAndNestedUpdateCompleteSafely", noUnexpectedExceptions && timerCountsCorrect },
					{ "newTimerUsesOnlyOuterUpdatesAfterRegistration", followUpFiredAtCorrectTime },
					{ "callbackOrderMatchesExpected", callbackOrder == expectedCallbackOrder },
					{ "listenerHeldThroughCallbackAndReleasedAfterward", listenerLifetimeCorrect },
					{ "pendingListenerDoesNotRun", pendingListenerRejected }
				} },
				{ "callbackOrder", callbackOrder },
				{ "caseId", "E17" },
				{ "counts", {
					{ "outerCallbackInvocations", outerCallbackCount },
					{ "followUpCallbackInvocations", followUpCallbackCount },
					{ "followUpCallbacksAfterOuterUpdate", followUpCallbackCountAfterOuterUpdate },
					{ "followUpCallbacksAfterFirstUpdate", followUpCallbackCountAfterFirstUpdate },
					{ "listenerLifetimeCallbackInvocations", lifetimeCallbackCount },
					{ "listenerAliveInsideCallback", listenerAliveInsideCallback },
					{ "listenerExpiredAfterCallback", listenerExpiredAfterCallback },
					{ "pendingListenerCallbackInvocations", pendingListenerCallbackCount },
					{ "timersAfterClear", timerCountAfterClear },
					{ "timersAfterOuterUpdate", timerCountAfterOuterUpdate },
					{ "timersAfterFirstFollowUpUpdate", timerCountAfterFirstFollowUpUpdate },
					{ "timersAfterSecondFollowUpUpdate", timerCountAfterSecondFollowUpUpdate },
					{ "timersBeforeOuterUpdate", timerCountBeforeOuterUpdate },
					{ "timersBeforeLifetimeUpdate", timerCountBeforeLifetimeUpdate },
					{ "timersAfterLifetimeUpdate", timerCountAfterLifetimeUpdate },
					{ "timersBeforePendingListenerUpdate", timerCountBeforePendingListenerUpdate },
					{ "timersAfterPendingListenerUpdate", timerCountAfterPendingListenerUpdate },
					{ "timersBeforeClear", timerCountBeforeClear },
					{ "timersAfterCleanup", timerCountAfterCleanup }
				} },
				{ "errors", {
					{ "nestedUpdateError", nestedUpdateError },
					{ "outerUpdateError", outerUpdateError },
					{ "firstFollowUpUpdateError", firstFollowUpUpdateError },
					{ "secondFollowUpUpdateError", secondFollowUpUpdateError },
					{ "listenerLifetimeUpdateError", listenerLifetimeUpdateError },
					{ "pendingListenerUpdateError", pendingListenerUpdateError }
				} },
				{ "expected", {
					{ "callbackOrder", expectedCallbackOrder },
					{ "followUpCallbackInvocationsAfterOuterUpdate", 0 },
					{ "followUpCallbackInvocationsAfterFirstUpdate", 0 },
					{ "followUpCallbackInvocationsAfterSecondUpdate", 1 },
					{ "timersAfterOuterUpdate", 1 },
					{ "timersAfterFirstFollowUpUpdate", 1 },
					{ "timersAfterSecondFollowUpUpdate", 0 },
					{ "listenerAliveInsideCallback", true },
					{ "listenerExpiredAfterCallback", true },
					{ "pendingListenerCallbackInvocations", 0 }
				} },
				{ "actual", {
					{ "callbackOrder", callbackOrder },
					{ "followUpCallbackInvocationsAfterOuterUpdate", followUpCallbackCountAfterOuterUpdate },
					{ "followUpCallbackInvocationsAfterFirstUpdate", followUpCallbackCountAfterFirstUpdate },
					{ "followUpCallbackInvocationsAfterSecondUpdate", followUpCallbackCount },
					{ "timersAfterOuterUpdate", timerCountAfterOuterUpdate },
					{ "timersAfterFirstFollowUpUpdate", timerCountAfterFirstFollowUpUpdate },
					{ "timersAfterSecondFollowUpUpdate", timerCountAfterSecondFollowUpUpdate },
					{ "listenerAliveInsideCallback", listenerAliveInsideCallback },
					{ "listenerExpiredAfterCallback", listenerExpiredAfterCallback },
					{ "pendingListenerCallbackInvocations", pendingListenerCallbackCount }
				} },
				{ "expectedCallbackOrder", expectedCallbackOrder },
				{ "handles", {
					{ "outerTimer", outerHandle.GetTimerKey() },
					{ "followUpTimer", followUpHandleKey },
					{ "listenerLifetimeTimer", lifetimeHandle.GetTimerKey() },
					{ "pendingListenerTimer", pendingListenerHandle.GetTimerKey() }
				} },
				{ "input", {
					{ "outerUpdateDeltaSeconds", outerUpdateDeltaSeconds },
					{ "nestedUpdateDeltaSeconds", nestedUpdateDeltaSeconds },
					{ "followUpTimerDurationSeconds", followUpTimerDurationSeconds },
					{ "listenerLifetimeOwnerId", lifetimeOwnerId },
					{ "pendingOwnerId", pendingOwnerId },
					{ "listenerLifetimeTimerDurationSeconds", 0.f },
					{ "listenerLifetimeUpdateDeltaSeconds", 0.f },
					{ "pendingListenerTimerDurationSeconds", 0.f },
					{ "pendingListenerUpdateDeltaSeconds", 0.f },
					{ "followUpUpdateDeltasSeconds", { outerUpdateDeltaSeconds, outerUpdateDeltaSeconds } }
				} },
				{ "passed", outerCallbackCount == 1 && timerCountBeforeClear >= timerCountAfterClear &&
					noUnexpectedExceptions && followUpFiredAtCorrectTime && timerCountsCorrect &&
					callbackOrder == expectedCallbackOrder && listenerLifetimeCorrect && pendingListenerRejected },
				{ "scenario", "luna.e17.clear_add_and_nested_update" },
				{ "schemaVersion", 1 }
			};
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
		Json e16Artifact{
			{ "caseId", "E16" },
			{ "passed", false },
			{ "scenario", "luna.e16.one_shot_throw_and_second_update" },
			{ "status", "skipped: production scene did not complete" },
			{ "schemaVersion", 1 }
		};
		Json e17Artifact{
			{ "caseId", "E17" },
			{ "passed", false },
			{ "scenario", "luna.e17.clear_add_and_nested_update" },
			{ "status", "skipped: production scene did not complete" },
			{ "schemaVersion", 1 }
		};
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
		if (setupError.empty() && runtimeError.empty() && levelStarted)
		{
			try
			{
				const std::weak_ptr<Object> sceneListener{ level };
				e16Artifact = RunOneShotThrowScenario(TimerManager::GetGlobalTimerManager(), sceneListener);
				e17Artifact = RunClearAddNestedUpdateScenario(TimerManager::GetGlobalTimerManager(), sceneListener);
			}
			catch (const std::exception& exception)
			{
				runtimeError = exception.what();
			}
			catch (...)
			{
				runtimeError = "Unknown exception while running TimerManager callback contract scenarios";
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
		const bool schedulerGateExceeded =
			combinedP95Microseconds / 1000.0 > TimerUpdateBudgetMilliseconds;
		const std::filesystem::path absoluteArtifactPath = std::filesystem::absolute(artifactPath);
		const std::filesystem::path e16ArtifactPath = absoluteArtifactPath.parent_path() / "luna-e16-timer-one-shot.json";
		const std::filesystem::path e17ArtifactPath = absoluteArtifactPath.parent_path() / "luna-e17-timer-clear-add-nested-update.json";
		const bool e16ArtifactWritten = WriteArtifact(e16ArtifactPath, e16Artifact);
		const bool e17ArtifactWritten = WriteArtifact(e17ArtifactPath, e17Artifact);
		const bool callbackScenariosPassed = e16Artifact.value("passed", false) && e17Artifact.value("passed", false);
		const bool behaviorArtifactsWritten = e16ArtifactWritten && e17ArtifactWritten;
		const bool sceneMeasurementCompleted = setupError.empty() && runtimeError.empty() && levelStarted &&
			metricSamplesComplete;
		const bool measurementCompleted = sceneMeasurementCompleted && callbackScenariosPassed && behaviorArtifactsWritten;

		Json artifact{
			{ "assertions", {
				{ "bothApplicationTimerManagersMeasured", metricSamplesComplete },
				{ "e16OneShotConsumedBeforeThrowAndNotRetried", e16Artifact.value("passed", false) },
				{ "e17ClearAddAndNestedUpdateRemainSafe", e17Artifact.value("passed", false) },
				{ "realArenaLevelStartedAndTicked", levelStarted && completedFrames == SimulationFrameCount },
				{ "runtimeContentBootstrapSucceeded", setupError.empty() }
			} },
			{ "callbackContractScenarios", {
				{ "e16", e16Artifact },
				{ "e17", e17Artifact },
				{ "passed", callbackScenariosPassed }
			} },
			{ "contractArtifacts", {
				{ "e16", { { "path", e16ArtifactPath.string() }, { "written", e16ArtifactWritten } } },
				{ "e17", { { "path", e17ArtifactPath.string() }, { "written", e17ArtifactWritten } } }
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
			<< " combinedUpdateP95Us=" << combinedP95Microseconds
			<< " callbackScenariosPassed=" << callbackScenariosPassed << '\n';
		if (!measurementCompleted)
		{
			std::cerr << "TimerManager scene E2E measurement failed; see the artifact for outcomes.\n";
			return 1;
		}
		std::cout << "TimerManager scene E2E measurement completed.\n";
		return 0;
	}
}
