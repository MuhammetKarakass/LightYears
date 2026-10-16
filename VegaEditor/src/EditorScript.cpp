#include "editor/EditorScript.h"
#include "editor/ScriptExecution.h"
#include "editor/EditorExtensions.h"
#include <framework/World.h>

namespace vega
{
	EditorScript::EditorScript(const std::string& path, GameExportLayout exportLayout, const ScriptCommandRegistry& commands)
		: mReport{ path + ".report.txt", std::ios::trunc }
		, mCommands{ commands }
	{
		mState.mExportLayout = std::move(exportLayout);
		std::ifstream input{ path };
		if (!input || !mReport)
		{
			Report("--editor-script " + path, false, "Cannot open script or report");
			mState.mFinished = true;
			return;
		}
		for (std::string line; std::getline(input, line);)
		{
			if (!line.empty() && line.back() == '\r') line.pop_back();
			mLines.push_back(line);
		}
		if (input.bad())
		{
			Report("--editor-script " + path, false, "Cannot read script");
			mState.mFinished = true;
		}
	}

	void EditorScript::Report(const std::string& line, bool success, const std::string& detail, bool handled)
	{
		LY_GAME_INFO("Editor script: %s %s -- %s", success ? "OK" : "FAIL", line.c_str(), detail.c_str());
		mReport << (success ? "OK " : "FAIL ") << line << " -- " << detail << '\n';
		mReport.flush();
		if ((!success && !handled) || !mReport) mFailed = true;
	}

	bool EditorScript::Step(EditorContext& context, CommandHistory& history, sf::RenderWindow& window)
	{
		if (mState.mFinished) return true;
		if (mState.mExportFuture.valid())
		{
			// Wait for script ordering, but return each frame so rendering and input continue.
			if (mState.mExportFuture.wait_for(std::chrono::seconds{ 0 }) != std::future_status::ready) return false;
			try { mState.mLastExport = mState.mExportFuture.get(); }
			catch (const std::exception& error) { mState.mLastExport = GameExportResult{}; mState.mLastExport->error = error.what(); }
			Report(mState.mExportLine, mState.mLastExport->success, DescribeGameExport(*mState.mLastExport));
			return false;
		}
		if (!context.world)
		{
			// A failed world startup must not leave an unattended editor running forever.
			if (++mStartupFrames < 300) return false;
			Report("startup", false, "No live world after 300 frames");
			mState.mFinished = true;
			return true;
		}
		if (!mWorldReady)
		{
			// BeginPlay spawns pending actors after the world's first tick slot. Let them become live before pause.
			mWorldReady = true;
			return false;
		}
		if (mState.mWaitFrames)
		{
			--mState.mWaitFrames;
			return false;
		}
		EditorScriptParseResult parsed;
		if (std::chrono::steady_clock::now() < mState.mWaitUntil) return false;
		std::string line;
		do
		{
			if (mNextLine == mLines.size())
			{
				if (mState.mPendingPropertyError) mFailed = true;
				mState.mFinished = true;
				return true;
			}
			line = mLines[mNextLine++];
			parsed = ParseEditorScriptLine(line, mCommands);
		} while (parsed.IsIgnored());
		if (!parsed.error.empty())
		{
			Report(line, false, parsed.error);
			return false;
		}
		const auto& command = *parsed.command;
		// Only an immediately following assertion may consume a refused property write.
		if (mState.mPendingPropertyError && command.operation != "expect-prop-error")
		{
			mFailed = true;
			mState.mPendingPropertyError.reset();
		}
		ScriptExecution execution{context, history, window, mState, mCommands, line};
		const auto* definition = mCommands.Find(command.operation);
		definition->execute(execution, command);
		if (execution.deferReport) return false;
		const bool success = execution.success;
		auto& detail = execution.detail;
		if (detail.empty()) detail = success ? "completed" : "No matching actor, selection, remembered location or history entry";
		if (success && (command.operation == "select" || command.operation == "select-type"))
		{
			const auto selected = context.selection.Get();
			const auto location = selected->GetActorLocation();
			detail = ActorTypeName(*selected) + " #" + std::to_string(selected->GetUniqueID()) +
				" at " + std::to_string(location.x) + " " + std::to_string(location.y);
		}
		Report(line, success, detail, mState.mPendingPropertyError.has_value());
		return mState.mFinished;
	}
}
