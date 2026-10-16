#pragma once

#include "editor/GameExporter.h"
#include "editor/ScriptCommandRegistry.h"
#include <SFML/System/Vector2.hpp>
#include <fstream>
#include <functional>
#include <optional>
#include <string>
#include <vector>
#include <chrono>

namespace sf { class RenderWindow; }

namespace vega
{
	struct EditorContext;
	class CommandHistory;

	struct EditorScriptCommand
	{
		std::string operation;
		std::string text;
		std::string property;
		std::vector<std::string> values;
		unsigned int number = 0;
		sf::Vector2f location{};
		sf::Vector2f rectangleEnd{};
		float degrees = 0.f;
		float factor = 1.f;
		float tolerance = 0.5f;
		bool overwrite = false;
	};
	struct EditorScriptParseResult
	{
		std::optional<EditorScriptCommand> command;
		std::string error;
		bool IsIgnored() const { return !command && error.empty(); }
	};

	// Pure parsing: no world, file, logging, UI or window access.
	EditorScriptParseResult ParseEditorScriptLine(const std::string& line, const ScriptCommandRegistry& registry);

	struct EditorScriptState
	{
		std::optional<std::string> mPendingPropertyError;
		unsigned int mWaitFrames = 0;
		std::optional<sf::Vector2f> mRememberedLocation;
		std::optional<float> mRememberedRotation;
		std::chrono::steady_clock::time_point mWaitUntil{};
		bool mFinished = false;
		GameExportLayout mExportLayout;
		std::optional<GameExportResult> mLastExport;
		std::future<GameExportResult> mExportFuture;
		std::string mExportLine;
	};

	class EditorScript
	{
	public:
		EditorScript(const std::string& path, GameExportLayout exportLayout, const ScriptCommandRegistry& commands);
		// Called once per completed UI frame; true requests orderly application shutdown.
		bool Step(EditorContext& context, CommandHistory& history, sf::RenderWindow& window);
		int ExitCode() const { return mState.mFinished && !mFailed ? 0 : 1; }
	private:
		void Report(const std::string& line, bool success, const std::string& detail, bool handled = false);
		std::vector<std::string> mLines;
		std::ofstream mReport;
		std::size_t mNextLine = 0;
		unsigned int mStartupFrames = 0;
		bool mFailed = false;
		bool mWorldReady = false;
		EditorScriptState mState;
		const ScriptCommandRegistry& mCommands;
	};
}
