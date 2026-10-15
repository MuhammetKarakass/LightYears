#pragma once

#include "editor/GameExporter.h"
#include <SFML/System/Vector2.hpp>
#include <fstream>
#include <optional>
#include <string>
#include <vector>
#include <chrono>

namespace sf { class RenderWindow; }

namespace vega
{
	struct EditorContext;
	class CommandHistory;

	enum class ScriptOperation
	{
		Tool, Snap, Grid, Colliders, ContentView,
		Pause, Play, Wait, Select, SelectType, Move, Rotate, Undo, Redo, SetProp, ExpectProp,
		ExpectSelected, ExpectLocation, ExpectCount, Capture, Quit,
		RememberLocation, ExpectRememberedLocation, Camera, Focus, Zoom, Pan, ExpectVisible, ExpectCamera,
		OpenJson, JsonSet, ExpectJson, JsonSave, JsonRevert, ExpectJsonDirty, Export, ExpectExportOk,
		AddScript, SetScriptProp, ExpectScriptProp, ExpectScriptError, WaitSeconds, RememberRotation, ExpectRotationChanged
	};
	struct EditorScriptCommand
	{
		ScriptOperation operation = ScriptOperation::Quit;
		std::string text;
		std::string property;
		std::vector<std::string> values;
		unsigned int number = 0;
		sf::Vector2f location{};
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
	EditorScriptParseResult ParseEditorScriptLine(const std::string& line);

	class EditorScript
	{
	public:
		EditorScript(const std::string& path, GameExportLayout exportLayout);
		// Called once per completed UI frame; true requests orderly application shutdown.
		bool Step(EditorContext& context, CommandHistory& history, sf::RenderWindow& window);
		int ExitCode() const { return mFinished && !mFailed ? 0 : 1; }
	private:
		void Report(const std::string& line, bool success, const std::string& detail);
		std::vector<std::string> mLines;
		std::ofstream mReport;
		std::size_t mNextLine = 0;
		unsigned int mWaitFrames = 0;
		unsigned int mStartupFrames = 0;
		std::optional<sf::Vector2f> mRememberedLocation;
		std::optional<float> mRememberedRotation;
		std::chrono::steady_clock::time_point mWaitUntil{};
		bool mFailed = false;
		bool mFinished = false;
		bool mWorldReady = false;
		GameExportLayout mExportLayout;
		std::optional<GameExportResult> mLastExport;
	};
}
