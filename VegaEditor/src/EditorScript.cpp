#include "editor/EditorScript.h"
#include "editor/EditorCommands.h"
#include <framework/World.h>
#include <SFML/Graphics.hpp>
#include <cmath>
#include <filesystem>

namespace vega
{
	EditorScript::EditorScript(const std::string& path)
		: mReport{ path + ".report.txt", std::ios::trunc }
	{
		std::ifstream input{ path };
		if (!input || !mReport)
		{
			Report("--editor-script " + path, false, "Cannot open script or report");
			mFinished = true;
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
			mFinished = true;
		}
	}

	void EditorScript::Report(const std::string& line, bool success, const std::string& detail)
	{
		LY_GAME_INFO("Editor script: %s %s -- %s", success ? "OK" : "FAIL", line.c_str(), detail.c_str());
		mReport << (success ? "OK " : "FAIL ") << line << " -- " << detail << '\n';
		mReport.flush();
		if (!success || !mReport) mFailed = true;
	}

	bool EditorScript::Step(EditorContext& context, CommandHistory& history, sf::RenderWindow& window)
	{
		if (mFinished) return true;
		if (!context.world)
		{
			// A failed world startup must not leave an unattended editor running forever.
			if (++mStartupFrames < 300) return false;
			Report("startup", false, "No live world after 300 frames");
			mFinished = true;
			return true;
		}
		if (!mWorldReady)
		{
			// BeginPlay spawns pending actors after the world's first tick slot. Let them become live before pause.
			mWorldReady = true;
			return false;
		}
		if (mWaitFrames)
		{
			--mWaitFrames;
			return false;
		}
		EditorScriptParseResult parsed;
		std::string line;
		do
		{
			if (mNextLine == mLines.size())
			{
				mFinished = true;
				return true;
			}
			line = mLines[mNextLine++];
			parsed = ParseEditorScriptLine(line);
		} while (parsed.IsIgnored());
		if (!parsed.error.empty())
		{
			Report(line, false, parsed.error);
			return false;
		}
		const auto& command = *parsed.command;
		auto actor = context.selection.Get();
		if (actor && actor->GetWorld() != context.world) actor.reset();
		bool success = false;
		std::string detail;
		const auto expectLocation = [&](sf::Vector2f expected)
		{
			if (!actor) return false;
			const auto actual = actor->GetActorLocation();
			detail = "actual " + std::to_string(actual.x) + " " + std::to_string(actual.y) +
				" expected " + std::to_string(expected.x) + " " + std::to_string(expected.y);
			return std::abs(actual.x - expected.x) <= command.tolerance && std::abs(actual.y - expected.y) <= command.tolerance;
		};
		switch (command.operation)
		{
		case ScriptOperation::Pause: success = SetPaused{ true }.Execute(context); break;
		case ScriptOperation::Camera: success = context.SetEditorCamera(command.text == "editor"); break;
		case ScriptOperation::Focus: success = context.FocusSelection(); break;
		case ScriptOperation::Zoom:
		case ScriptOperation::Pan:
			if (context.SetEditorCamera(true))
			{
				if (command.operation == ScriptOperation::Zoom)
					context.camera->ZoomAt(command.factor, context.world->GetWorldView().getCenter());
				else context.camera->Pan(command.location);
				context.ApplyCamera();
				success = true;
			}
			break;
		case ScriptOperation::ExpectCamera:
			success = context.world->GetViewOverride().has_value() == (command.text == "editor");
			break;
		case ScriptOperation::ExpectVisible:
		{
			const auto view = context.world->GetWorldView();
			success = actor && actor->GetActorGlobalBounds().findIntersection(
				sf::FloatRect{ view.getCenter() - view.getSize() * 0.5f, view.getSize() }).has_value();
			break;
		}
		case ScriptOperation::Play: success = SetPaused{ false }.Execute(context); break;
		case ScriptOperation::Wait: mWaitFrames = command.number; success = true; break;
		case ScriptOperation::Select: success = SelectActor{ command.number }.Execute(context); break;
		case ScriptOperation::SelectType:
		{
			unsigned int index = 0;
			for (const auto& weak : context.world->GetActorsByType<ly::Actor>())
			{
				if (auto candidate = weak.lock(); candidate && !candidate->GetIsPendingDestroy() && ActorTypeName(*candidate) == command.text)
				{
					if (index++ == command.number)
					{
						success = SelectActor{ candidate->GetUniqueID() }.Execute(context);
						break;
					}
				}
			}
			break;
		}
		case ScriptOperation::Move:
			if (actor) success = history.Execute(std::make_unique<SetActorLocation>(actor->GetUniqueID(), command.location));
			break;
		case ScriptOperation::Rotate:
			if (actor) success = history.Execute(std::make_unique<SetActorRotation>(actor->GetUniqueID(), command.degrees));
			break;
		case ScriptOperation::Undo: success = history.Undo(); break;
		case ScriptOperation::Redo: success = history.Redo(); break;
		case ScriptOperation::ExpectSelected:
			success = actor && ActorTypeName(*actor) == command.text;
			detail = "selected " + (actor ? ActorTypeName(*actor) : std::string{ "none" });
			break;
		case ScriptOperation::ExpectLocation: success = expectLocation(command.location); break;
		case ScriptOperation::ExpectCount:
		{
			unsigned int count = 0;
			for (const auto& weak : context.world->GetActorsByType<ly::Actor>())
			{
				if (auto candidate = weak.lock(); candidate && !candidate->GetIsPendingDestroy() && ActorTypeName(*candidate) == command.text) ++count;
			}
			success = count == command.number;
			detail = "count " + std::to_string(count) + " expected " + std::to_string(command.number);
			break;
		}
		case ScriptOperation::RememberLocation:
			if (actor)
			{
				mRememberedLocation = actor->GetActorLocation();
				detail = "remembered " + std::to_string(mRememberedLocation->x) + " " + std::to_string(mRememberedLocation->y);
				success = true;
			}
			break;
		case ScriptOperation::ExpectRememberedLocation:
			if (mRememberedLocation) success = expectLocation(*mRememberedLocation);
			break;
		case ScriptOperation::Capture:
		{
			const std::filesystem::path path{ command.text };
			std::error_code error;
			if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path(), error);
			if (!error)
			{
				// ImGui has rendered, so the active back buffer includes the complete editor UI.
				sf::Texture texture{ window.getSize() };
				texture.update(window);
				success = texture.copyToImage().saveToFile(path);
			}
			detail = "capture " + command.text;
			break;
		}
		case ScriptOperation::Quit: mFinished = true; success = true; break;
		}
		if (detail.empty()) detail = success ? "completed" : "No matching actor, selection, remembered location or history entry";
		if (success && (command.operation == ScriptOperation::Select || command.operation == ScriptOperation::SelectType))
		{
			const auto selected = context.selection.Get();
			const auto location = selected->GetActorLocation();
			detail = ActorTypeName(*selected) + " #" + std::to_string(selected->GetUniqueID()) +
				" at " + std::to_string(location.x) + " " + std::to_string(location.y);
		}
		Report(line, success, detail);
		return mFinished;
	}
}
