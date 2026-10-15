#pragma once

#include <SFML/System/Vector2.hpp>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

namespace sf { class RenderWindow; }

namespace vega
{
	struct EditorContext;
	class CommandHistory;

	enum class ScriptOperation
	{
		Pause, Play, Wait, Select, SelectType, Move, Rotate, Undo, Redo,
		ExpectSelected, ExpectLocation, ExpectCount, Capture, Quit,
		RememberLocation, ExpectRememberedLocation, Camera, Focus, Zoom, Pan, ExpectVisible, ExpectCamera
	};
	struct EditorScriptCommand
	{
		ScriptOperation operation = ScriptOperation::Quit;
		std::string text;
		unsigned int number = 0;
		sf::Vector2f location{};
		float degrees = 0.f;
		float factor = 1.f;
		float tolerance = 0.5f;
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
		explicit EditorScript(const std::string& path);
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
		bool mFailed = false;
		bool mFinished = false;
		bool mWorldReady = false;
	};
}
