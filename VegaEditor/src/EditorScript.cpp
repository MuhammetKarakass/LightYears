#include "editor/EditorScript.h"
#include "editor/EditorCommands.h"
#include <framework/World.h>
#include <SFML/Graphics.hpp>
#include <cmath>
#include <filesystem>
#include <charconv>
#include <locale>
#include <sstream>

namespace vega
{
	namespace
	{
		bool ParsePropertyValue(const ly::reflect::PropertyInfo& property, const std::vector<std::string>& arguments,
			bool expect, ly::reflect::PropertyValue& value, float& tolerance)
		{
			using ly::reflect::PropertyKind;
			const std::size_t count = property.kind == PropertyKind::Vector2 ? 2u : property.kind == PropertyKind::Color ? 4u : 1u;
			if (arguments.size() != count && (!expect || arguments.size() != count + 1)) return false;
			const auto real = [&](std::size_t index, float& number)
			{
				std::istringstream stream{ arguments[index] };
				stream.imbue(std::locale::classic());
				return (stream >> number) && stream.eof() && std::isfinite(number);
			};
			const auto integer = [&](std::size_t index, int& number)
			{
				const auto& text = arguments[index];
				const auto result = std::from_chars(text.data(), text.data() + text.size(), number);
				return result.ec == std::errc{} && result.ptr == text.data() + text.size();
			};
			if (arguments.size() > count && (!real(count, tolerance) || tolerance < 0.f)) return false;
			switch (property.kind)
			{
			case PropertyKind::Bool:
				if (arguments[0] != "true" && arguments[0] != "false" && arguments[0] != "1" && arguments[0] != "0") return false;
				value = arguments[0] == "true" || arguments[0] == "1";
				return true;
			case PropertyKind::Int:
			case PropertyKind::Enum:
			{
				int number = 0;
				if (!integer(0, number)) return false;
				value = number;
				return true;
			}
			case PropertyKind::Float:
			{
				float number = 0.f;
				if (!real(0, number)) return false;
				value = number;
				return true;
			}
			case PropertyKind::Vector2:
			{
				sf::Vector2f vector;
				if (!real(0, vector.x) || !real(1, vector.y)) return false;
				value = vector;
				return true;
			}
			case PropertyKind::Color:
			{
				int channels[4]{};
				for (std::size_t index = 0; index < 4; ++index)
					if (!integer(index, channels[index]) || channels[index] < 0 || channels[index] > 255) return false;
				value = sf::Color{ static_cast<std::uint8_t>(channels[0]), static_cast<std::uint8_t>(channels[1]),
					static_cast<std::uint8_t>(channels[2]), static_cast<std::uint8_t>(channels[3]) };
				return true;
			}
			case PropertyKind::String: value = arguments[0]; return true;
			}
			return false;
		}

		bool PropertyEquals(const ly::reflect::PropertyValue& actual, const ly::reflect::PropertyValue& expected, float tolerance)
		{
			if (actual.index() != expected.index()) return false;
			if (const auto* number = std::get_if<float>(&actual)) return std::abs(*number - std::get<float>(expected)) <= tolerance;
			if (const auto* vector = std::get_if<sf::Vector2f>(&actual))
			{
				const auto other = std::get<sf::Vector2f>(expected);
				return std::abs(vector->x - other.x) <= tolerance && std::abs(vector->y - other.y) <= tolerance;
			}
			return actual == expected;
		}
	}

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
		case ScriptOperation::SetProp:
		case ScriptOperation::ExpectProp:
		{
			const auto target = actor ? FindPropertyTarget(*actor, command.text) : ReflectedTarget{};
			const auto* property = target.type ? target.type->FindProperty(command.property) : nullptr;
			ly::reflect::PropertyValue value;
			float tolerance = command.tolerance;
			const bool expect = command.operation == ScriptOperation::ExpectProp;
			if (!property) detail = "Unknown target or property: " + command.text + " / " + command.property;
			else if (!ParsePropertyValue(*property, command.values, expect, value, tolerance)) detail = "Invalid property value or tolerance";
			else if (expect)
			{
				success = PropertyEquals(property->getter(target.object), value, tolerance);
				detail = success ? "Property matched" : "Property value differs";
			}
			else
			{
				success = history.Execute(std::make_unique<SetPropertyCommand>(actor->GetUniqueID(), target.componentIndex, property->name, value));
				detail = success ? "Property set" : "Read-only property or refused value";
			}
			break;
		}
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
