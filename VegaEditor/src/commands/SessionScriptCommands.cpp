#include "editor/ScriptExecution.h"
#include "editor/EditorExtensions.h"
#include <framework/World.h>
#include <SFML/Graphics.hpp>
#include <cmath>
#include <filesystem>

namespace vega
{
	namespace
	{
		EditorScriptParseResult DecodeWaitSeconds(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			const auto real = [&arguments](std::size_t i, float& v) { return arguments.Real(i, v); };
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 2 || !real(1, command.factor) || command.factor < 0.f || command.factor > 86400.f) return bad();
			return {command, {}};
		}
		EditorScriptParseResult DecodePause(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 1) return bad();
			return {command, {}};
		}
		EditorScriptParseResult DecodePlay(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 1) return bad();
			return {command, {}};
		}
		EditorScriptParseResult DecodeUndo(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 1) return bad();
			return {command, {}};
		}
		EditorScriptParseResult DecodeRedo(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 1) return bad();
			return {command, {}};
		}
		EditorScriptParseResult DecodeQuit(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 1) return bad();
			return {command, {}};
		}
		EditorScriptParseResult DecodeRememberLocation(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 1) return bad();
			return {command, {}};
		}
		EditorScriptParseResult DecodeRememberRotation(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 1) return bad();
			return {command, {}};
		}
		EditorScriptParseResult DecodeExpectRotationChanged(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 1) return bad();
			return {command, {}};
		}
		EditorScriptParseResult DecodeExpectRememberedLocation(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 1) return bad();
			return {command, {}};
		}
		EditorScriptParseResult DecodeWait(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			const auto integer = [&arguments](std::size_t i, unsigned int& v) { return arguments.Integer(i, v); };
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 2 || !integer(1, command.number)) return bad();
			return {command, {}};
		}
		EditorScriptParseResult DecodeExpectSelected(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			const auto& verb = arguments.tokens[0];
			const auto integer = [&arguments](std::size_t i, unsigned int& v) { return arguments.Integer(i, v); };
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != (verb == "expect-count" ? 3u : 2u)) return bad();
			command.text = tokens[1];
			if (verb == "expect-count")
			{
				if (!integer(2, command.number)) return bad();
			}
			else command.operation = verb == "capture" ? "capture" : "expect-selected";
			return {command, {}};
		}
		void ExecuteWaitSeconds(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& success = execution.success;
			auto& mWaitUntil = execution.state.mWaitUntil;
			mWaitUntil = std::chrono::steady_clock::now() +
			    std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<float>{command.factor});
			success = true;
		}
		void ExecutePause(ScriptExecution& execution, const EditorScriptCommand&)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			success = SetPaused{true}.Execute(context);
		}
		void ExecutePlay(ScriptExecution& execution, const EditorScriptCommand&)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			success = SetPaused{false}.Execute(context);
		}
		void ExecuteUndo(ScriptExecution& execution, const EditorScriptCommand&)
		{
			auto& context = execution.context;
			auto& history = execution.history;
			auto& success = execution.success;
			success = context.extensions && context.extensions->History() ? context.extensions->History()->Undo() : history.Undo();
		}
		void ExecuteRedo(ScriptExecution& execution, const EditorScriptCommand&)
		{
			auto& context = execution.context;
			auto& history = execution.history;
			auto& success = execution.success;
			success = context.extensions && context.extensions->History() ? context.extensions->History()->Redo() : history.Redo();
		}
		void ExecuteQuit(ScriptExecution& execution, const EditorScriptCommand&)
		{
			auto& success = execution.success;
			auto& mFinished = execution.state.mFinished;
			mFinished = true;
			success = true;
		}
		void ExecuteRememberLocation(ScriptExecution& execution, const EditorScriptCommand&)
		{
			auto& success = execution.success;
			auto& detail = execution.detail;
			auto& mRememberedLocation = execution.state.mRememberedLocation;
			auto actor = execution.context.selection.Get();
			if (actor && actor->GetWorld() != execution.context.world) actor.reset();
			if (actor)
			{
				mRememberedLocation = actor->GetActorLocation();
				detail = "remembered " + std::to_string(mRememberedLocation->x) + " " + std::to_string(mRememberedLocation->y);
				success = true;
			}
		}
		void ExecuteRememberRotation(ScriptExecution& execution, const EditorScriptCommand&)
		{
			auto& success = execution.success;
			auto& mRememberedRotation = execution.state.mRememberedRotation;
			auto actor = execution.context.selection.Get();
			if (actor && actor->GetWorld() != execution.context.world) actor.reset();
			if (actor)
			{
				mRememberedRotation = actor->GetActorRotation();
				success = true;
			}
		}
		void ExecuteExpectRotationChanged(ScriptExecution& execution, const EditorScriptCommand&)
		{
			auto& success = execution.success;
			auto& detail = execution.detail;
			auto& mRememberedRotation = execution.state.mRememberedRotation;
			auto actor = execution.context.selection.Get();
			if (actor && actor->GetWorld() != execution.context.world) actor.reset();
			if (actor && mRememberedRotation)
			{
				const float rotation = actor->GetActorRotation();
				success = std::abs(std::remainder(rotation - *mRememberedRotation, 360.f)) > 0.01f;
				detail = "rotation " + std::to_string(rotation) + " before " + std::to_string(*mRememberedRotation);
			}
		}
		void ExecuteExpectRememberedLocation(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& detail = execution.detail;
			auto actor = execution.context.selection.Get();
			if (actor && actor->GetWorld() != execution.context.world) actor.reset();
			auto& success = execution.success;
			auto& mRememberedLocation = execution.state.mRememberedLocation;
			const auto expectLocation = [&](sf::Vector2f expected)
			{
				if (!actor) return false;
				const auto actual = actor->GetActorLocation();
				detail = "actual " + std::to_string(actual.x) + " " + std::to_string(actual.y) + " expected " + std::to_string(expected.x) + " " +
				    std::to_string(expected.y);
				return std::abs(actual.x - expected.x) <= command.tolerance && std::abs(actual.y - expected.y) <= command.tolerance;
			};
			if (mRememberedLocation) success = expectLocation(*mRememberedLocation);
		}
		void ExecuteWait(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& success = execution.success;
			auto& mWaitFrames = execution.state.mWaitFrames;
			mWaitFrames = command.number;
			success = true;
		}
		void ExecuteSelect(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			success = SelectActor{command.number}.Execute(context);
		}
		void ExecuteExpectSelected(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& success = execution.success;
			auto& detail = execution.detail;
			auto actor = execution.context.selection.Get();
			if (actor && actor->GetWorld() != execution.context.world) actor.reset();
			success = actor && ActorTypeName(*actor) == command.text;
			detail = "selected " + (actor ? ActorTypeName(*actor) : std::string{"none"});
		}
		void ExecuteCapture(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& window = execution.window;
			auto& success = execution.success;
			auto& detail = execution.detail;
			const std::filesystem::path path{command.text};
			std::error_code error;
			if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path(), error);
			if (!error)
			{
				// ImGui has rendered, so the active back buffer includes the complete editor UI.
				sf::Texture texture{window.getSize()};
				texture.update(window);
				success = texture.copyToImage().saveToFile(path);
			}
			detail = "capture " + command.text;
		}
		void ExecuteExpectCount(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			auto& detail = execution.detail;
			unsigned int count = 0;
			for (const auto& weak : context.world->GetActorsByTypeIncludingPending<vega::Actor>())
			{
				if (auto candidate = weak.lock(); candidate && !candidate->GetIsPendingDestroy() && ActorTypeName(*candidate) == command.text) ++count;
			}
			success = count == command.number;
			detail = "count " + std::to_string(count) + " expected " + std::to_string(command.number);
		}
	} // namespace
	void RegisterSessionScriptCommands(ScriptCommandRegistry& registry)
	{
		registry.Add({"wait-seconds", Schema("seconds:real"), "Wait a finite duration between zero and 86400 seconds.", ScriptMode::Any, DecodeWaitSeconds,
		    ExecuteWaitSeconds});
		registry.Add({"pause", Schema(""), "Pause the current world.", ScriptMode::Any, DecodePause, ExecutePause});
		registry.Add({"play", Schema(""), "Unpause the current world.", ScriptMode::Any, DecodePlay, ExecutePlay});
		registry.Add({"undo", Schema(""), "Undo in the active tool history or scene history.", ScriptMode::Any, DecodeUndo, ExecuteUndo});
		registry.Add({"redo", Schema(""), "Redo in the active tool history or scene history.", ScriptMode::Any, DecodeRedo, ExecuteRedo});
		registry.Add({"quit", Schema(""), "Finish the script and request orderly shutdown.", ScriptMode::Any, DecodeQuit, ExecuteQuit});
		registry.Add({"remember-location", Schema(""), "Remember the selected actor location for a later assertion.", ScriptMode::Any, DecodeRememberLocation,
		    ExecuteRememberLocation});
		registry.Add(
		    {"remember-rotation", Schema(""), "Remember the selected actor rotation.", ScriptMode::Any, DecodeRememberRotation, ExecuteRememberRotation});
		registry.Add({"expect-rotation-changed", Schema(""), "Assert that the selected actor rotation changed from its remembered value.", ScriptMode::Any,
		    DecodeExpectRotationChanged, ExecuteExpectRotationChanged});
		registry.Add({"expect-remembered-location", Schema(""), "Assert the selected actor matches its remembered location.", ScriptMode::Any,
		    DecodeExpectRememberedLocation, ExecuteExpectRememberedLocation});
		registry.Add({"wait", Schema("number:uint"), "Wait the supplied number of completed editor frames.", ScriptMode::Any, DecodeWait, ExecuteWait});
		registry.Add({"select", Schema("number:uint"), "Select an actor by numeric ID.", ScriptMode::Any, DecodeWait, ExecuteSelect});
		registry.Add(
		    {"expect-selected", Schema("type:string"), "Assert the selected actor type.", ScriptMode::Any, DecodeExpectSelected, ExecuteExpectSelected});
		registry.Add(
		    {"capture", Schema("path:string"), "Capture the complete editor window to an image file.", ScriptMode::Any, DecodeExpectSelected, ExecuteCapture});
		registry.Add({"expect-count", Schema("type:string count:uint"), "Assert the number of live actors of a type.", ScriptMode::Any, DecodeExpectSelected,
		    ExecuteExpectCount});
	}
} // namespace vega
