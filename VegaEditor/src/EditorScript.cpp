#include "editor/EditorScript.h"
#include "editor/EditorExtensions.h"
#include <framework/scene/SceneSettings.h>
#include <framework/debug/Profiler.h>
#include <algorithm>
#include "editor/EditorCommands.h"
#include "editor/JsonCommands.h"
#include "editor/EditorPrefabs.h"
#include <framework/World.h>
#include <framework/ProjectSettings.h>
#include <framework/scene/SceneEntityComponent.h>
#include <script/ScriptComponent.h>
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
			case PropertyKind::Flags:
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

	EditorScript::EditorScript(const std::string& path, GameExportLayout exportLayout)
		: mReport{ path + ".report.txt", std::ios::trunc }
		, mExportLayout{ std::move(exportLayout) }
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

	void EditorScript::Report(const std::string& line, bool success, const std::string& detail, bool handled)
	{
		LY_GAME_INFO("Editor script: %s %s -- %s", success ? "OK" : "FAIL", line.c_str(), detail.c_str());
		mReport << (success ? "OK " : "FAIL ") << line << " -- " << detail << '\n';
		mReport.flush();
		if ((!success && !handled) || !mReport) mFailed = true;
	}

	bool EditorScript::Step(EditorContext& context, CommandHistory& history, sf::RenderWindow& window)
	{
		if (mFinished) return true;
		if (mExportFuture.valid())
		{
			// Wait for script ordering, but return each frame so rendering and input continue.
			if (mExportFuture.wait_for(std::chrono::seconds{ 0 }) != std::future_status::ready) return false;
			try { mLastExport = mExportFuture.get(); }
			catch (const std::exception& error) { mLastExport = GameExportResult{}; mLastExport->error = error.what(); }
			Report(mExportLine, mLastExport->success, DescribeGameExport(*mLastExport));
			return false;
		}
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
		if (std::chrono::steady_clock::now() < mWaitUntil) return false;
		std::string line;
		do
		{
			if (mNextLine == mLines.size())
			{
				if (mPendingPropertyError) mFailed = true;
				mFinished = true;
				return true;
			}
			line = mLines[mNextLine++];
			parsed = ParseEditorScriptLine(line, [&](const std::string& name) { return context.extensions && context.extensions->HasScriptCommand(name); });
		} while (parsed.IsIgnored());
		if (!parsed.error.empty())
		{
			Report(line, false, parsed.error);
			return false;
		}
		const auto& command = *parsed.command;
		// Only an immediately following assertion may consume a refused property write.
		if (mPendingPropertyError && command.operation != ScriptOperation::ExpectPropError)
		{
			mFailed = true;
			mPendingPropertyError.reset();
		}
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
		case ScriptOperation::SetRef:
		case ScriptOperation::ExpectRef:
		case ScriptOperation::ListAdd:
		case ScriptOperation::ListRemove:
		case ScriptOperation::ListSet:
		case ScriptOperation::ExpectListSize:
		{
			const auto find = [&](const std::string& name) -> std::shared_ptr<ly::Actor>
			{
				if (const auto found = context.ResolveGuid(name)) return found;
				for (const auto& weak : context.world->GetActorsByTypeIncludingPending<ly::Actor>())
					if (const auto candidate = weak.lock(); candidate && !candidate->GetIsPendingDestroy())
						if (const auto* identity = candidate->FindComponent<ly::SceneEntityComponent>(); identity && identity->name == name) return candidate;
				return {};
			};
			auto owner = actor;
			auto target = actor ? FindPropertyTarget(*actor, command.text) : ReflectedTarget{};
			if (!target.object) { owner = find(command.text); if (owner) target = ResolvePropertyTarget(*owner, {}); }
			const auto* property = target.type ? target.type->FindProperty(command.property) : nullptr;
			if (!property) { detail = "Unknown target or property"; break; }
			auto value = property->getter(target.object);
			const auto parse = [&](const ly::reflect::PropertyInfo& descriptor, ly::reflect::PropertyValue& parsed)
			{
				if (descriptor.kind == ly::reflect::PropertyKind::ActorRef)
				{
					if (command.values.size() != 1) return false;
					const auto other = find(command.values[0]);
					if (!other && command.values[0] != "none") return false;
					parsed = other ? ly::ActorRef{ *other } : ly::ActorRef{};
					return true;
				}
				float tolerance = 0.f;
				return ParsePropertyValue(descriptor, command.values, false, parsed, tolerance);
			};
			if (command.operation == ScriptOperation::SetRef || command.operation == ScriptOperation::ExpectRef)
			{
				if (property->kind != ly::reflect::PropertyKind::ActorRef || !parse(*property, value)) { detail = "Expected actor reference and live target or none"; break; }
				if (command.operation == ScriptOperation::ExpectRef) { success = property->getter(target.object) == value; break; }
			}
			else
			{
				if (property->kind != ly::reflect::PropertyKind::List) { detail = "Expected list property"; break; }
				auto& list = std::get<ly::reflect::PropertyList>(value);
				if (command.operation == ScriptOperation::ExpectListSize) { success = list.size() == command.number; detail = "size " + std::to_string(list.size()); break; }
				if (command.operation != ScriptOperation::ListAdd && command.number >= list.size()) { detail = "List index out of range (zero-based)"; break; }
				if (command.operation == ScriptOperation::ListRemove) list.erase(list.begin() + command.number);
				else
				{
					ly::reflect::PropertyValue item;
					if (!parse(property->Element(), item)) { detail = "Invalid list element"; break; }
					if (command.operation == ScriptOperation::ListAdd) list.push_back(ly::reflect::ToScalar(item));
					else list[command.number] = ly::reflect::ToScalar(item);
				}
			}
			success = history.Execute(std::make_unique<SetPropertyCommand>(owner->GetUniqueID(), target.componentIdentity, property->name, value));
			detail = success ? "Property set" : context.propertyError;
			if (!success) mPendingPropertyError = detail;
			break;
		}
		case ScriptOperation::Parent:
		case ScriptOperation::Unparent:
		case ScriptOperation::ExpectParent:
		case ScriptOperation::ExpectWorldLocation:
		{
			const auto find = [&](const std::string& name) -> std::shared_ptr<ly::Actor>
			{
				if (const auto target = context.ResolveGuid(name)) return target;
				for (const auto& weak : context.world->GetActorsByTypeIncludingPending<ly::Actor>())
					if (const auto candidate = weak.lock(); candidate && !candidate->GetIsPendingDestroy())
						if (const auto* identity = candidate->FindComponent<ly::SceneEntityComponent>(); identity && identity->name == name) return candidate;
				return {};
			};
			const auto child = find(command.text), parent = find(command.property);
			if (!child) break;
			if (command.operation == ScriptOperation::ExpectWorldLocation)
			{
				const auto location = child->GetActorLocation();
				success = std::abs(location.x - command.location.x) <= command.tolerance && std::abs(location.y - command.location.y) <= command.tolerance;
				detail = "actual " + std::to_string(location.x) + " " + std::to_string(location.y);
			}
			else if (command.operation == ScriptOperation::ExpectParent)
				success = command.property == "none" ? child->GetParent() == nullptr : parent && child->GetParent() == parent.get();
			else if (command.operation == ScriptOperation::Unparent || parent)
				success = history.Execute(std::make_unique<ReparentActor>(child->FindComponent<ly::SceneEntityComponent>()->guid,
					parent ? parent->FindComponent<ly::SceneEntityComponent>()->guid : std::string{}));
			break;
		}
		case ScriptOperation::SkyOpen:
		case ScriptOperation::SkySet:
		case ScriptOperation::SkyDrag:
		case ScriptOperation::SkySave:
		case ScriptOperation::ExpectSky:
		case ScriptOperation::CaptureSky:
			success = context.extensions && context.extensions->ExecuteParsed(command.extensionName, context, command, detail); break;
		case ScriptOperation::GasSelectPlayer:
		case ScriptOperation::ExpectAbility:
		case ScriptOperation::ExpectEffect:
		case ScriptOperation::ExpectAttribute:
		case ScriptOperation::ExpectTag:
		case ScriptOperation::GasApplyEffect:
		case ScriptOperation::GasRemoveEffect:
		case ScriptOperation::GasResetCooldowns:
		case ScriptOperation::GasActivate:
		case ScriptOperation::GasPanel:
		case ScriptOperation::GasFilter:
			success = context.extensions && context.extensions->ExecuteParsed(command.extensionName, context, command, detail);
			break;
		case ScriptOperation::ClearConsole:
			if (context.clearConsole) { context.clearConsole(); success = true; }
			else if (context.console) { context.console->Clear(); success = true; }
			break;
		case ScriptOperation::ExpectNoErrors:
		case ScriptOperation::ExpectConsole:
			if (context.console)
			{
				const auto records = context.console->Snapshot();
				if (command.operation == ScriptOperation::ExpectNoErrors)
					success = std::none_of(records.begin(), records.end(), [](const auto& r) { return r.level >= ly::debug::LogLevel::Error; });
				else success = std::any_of(records.begin(), records.end(), [&](const auto& r)
				{
					const auto level = command.property == "Info" ? ly::debug::LogLevel::Info : command.property == "Warning" ? ly::debug::LogLevel::Warning : ly::debug::LogLevel::Error;
					return (r.level == level || (level == ly::debug::LogLevel::Error && r.level == ly::debug::LogLevel::Fatal)) && r.message.find(command.text) != std::string::npos;
				});
			}
			break;
		case ScriptOperation::Profiler:
			ly::debug::Profiler::SetEnabled(command.text == "on");
			success = ly::debug::Profiler::IsEnabled() == (command.text == "on");
			break;
		case ScriptOperation::Diagnostics:
			if (context.diagnosticsVisible) { context.diagnosticsVisible(command.text == "on"); success = true; }
			break;
		case ScriptOperation::ExpectProfileScope:
			for (const auto& frame : ly::debug::Profiler::CaptureFrames())
				for (const auto& scope : frame.scopes) if (scope.name == command.text) success = true;
			break;
		case ScriptOperation::CaptureProfile:
		{
			try
			{
				const auto requested = std::filesystem::u8path(command.text);
				const auto path = std::filesystem::weakly_canonical(requested.is_absolute() ? requested : mExportLayout.generatedRoot / "Saved" / requested);
				const auto inside = [](const auto& file, const auto& root)
				{
					if (root.empty()) return false;
					const auto relative = file.lexically_relative(std::filesystem::weakly_canonical(root));
					return !relative.empty() && *relative.begin() != "..";
				};
				if (inside(path, mExportLayout.assetRoot) || inside(path, mExportLayout.sourceAssetRoot) ||
					inside(path, mExportLayout.visualAssetsDir)) detail = "Profile captures cannot be written into assets";
				else success = ly::debug::Profiler::CaptureToFile(path, detail);
			}
			catch (const std::exception& error) { detail = error.what(); }
			break;
		}
		case ScriptOperation::SetSetting:
		case ScriptOperation::ResetSetting:
		{
			const auto value = command.operation == ScriptOperation::ResetSetting ? std::optional<nlohmann::json>{} :
				std::optional<nlohmann::json>{ nlohmann::json::parse(command.values[0]) };
			const auto& schema = ly::SceneSettingsSchema::Get();
			const auto* field = schema.Find(command.text);
			const bool noChange = context.sceneSettings && field &&
				(value ? field->Validate(*value).empty() && schema.Value(*context.sceneSettings, command.text) == *value : !context.sceneSettings->contains(command.text));
			success = !context.playMode && !context.transitioning && (noChange || history.Execute(std::make_unique<SetSceneSettingCommand>(command.text, value)));
			detail = success ? "Setting updated" : context.settingError;
			break;
		}
		case ScriptOperation::ExpectSetting:
			if (context.sceneSettings)
			{
				const auto actual = ly::SceneSettingsSchema::Get().Value(*context.sceneSettings, command.text);
				success = actual == nlohmann::json::parse(command.values[0]);
				detail = "Setting " + command.text + " = " + actual.dump();
			}
			break;
		case ScriptOperation::ExpectBackdrop:
			if (context.backdropState)
			{
				const auto actual = context.backdropState();
				success = actual == command.text;
				detail = "Procedural backdrop stop = " + actual;
			}
			break;
		case ScriptOperation::Extension:
			success = context.extensions && context.extensions->Execute(command.extensionName, context, history, command.values, detail);
			break;
		case ScriptOperation::OpenPrefab:
		case ScriptOperation::SavePrefab:
		case ScriptOperation::ClosePrefab:
			success = context.prefabAction && context.prefabAction(command.operation == ScriptOperation::OpenPrefab ? "open" : command.operation == ScriptOperation::SavePrefab ? "save" : "close", command.text);
			break;
		case ScriptOperation::CreatePrefab: success = CreatePrefab(context, history, command.text); break;
		case ScriptOperation::PlacePrefab:
			success = PlacePrefab(context, history, command.text, command.number ? command.location : context.world->GetWorldView().getCenter()); break;
		case ScriptOperation::CreateVariant: success = CreateVariant(context, command.text, command.property); break;
		case ScriptOperation::ReloadPrefabs: success = RefreshPrefabs(context, history); break;
		case ScriptOperation::ExpectPrefab:
			if (const auto* identity = actor ? actor->FindComponent<ly::SceneEntityComponent>() : nullptr)
				success = identity->prefab == (command.text == "none" ? "" : command.text);
			break;
		case ScriptOperation::ExpectOverridden:
			if (actor) success = PrefabPropertyOverridden(context, *actor, FindPropertyTarget(*actor, command.text), command.property) == command.overwrite;
			break;
		case ScriptOperation::ApplyPrefabProp:
		case ScriptOperation::RevertPrefabProp:
			if (actor) success = ChangePrefabProperty(context, history, FindPropertyTarget(*actor, command.text), command.property, command.operation == ScriptOperation::ApplyPrefabProp);
			break;
		case ScriptOperation::NewScene:
		case ScriptOperation::OpenScene:
		case ScriptOperation::SaveScene:
		case ScriptOperation::PlayMode:
		case ScriptOperation::Stop:
		{
			const std::string action = command.operation == ScriptOperation::NewScene ? "new" : command.operation == ScriptOperation::OpenScene ? "open" :
				command.operation == ScriptOperation::SaveScene ? "save" : command.operation == ScriptOperation::PlayMode ? "play" : "stop";
			success = context.sceneAction && context.sceneAction(action, command.text);
			detail = success ? "Scene " + action : context.sceneError;
			break;
		}
		case ScriptOperation::Create: success = CreateSceneEntity(context, history, command.text); break;
		case ScriptOperation::CreateMany:
		{
			const unsigned int columns = std::max(1u, static_cast<unsigned int>(std::ceil(std::sqrt(command.number))));
			const auto centre = context.world->GetWorldView().getCenter();
			success = true;
			for (unsigned int index = 0; index < command.number; ++index)
			{
				if (!CreateSceneEntity(context, history, command.text)) { success = false; break; }
				const auto created = context.selection.Get();
				const sf::Vector2f location = centre + sf::Vector2f{
					(static_cast<float>(index % columns) - (columns - 1) * 0.5f) * 48.f,
					(static_cast<float>(index / columns) - (columns - 1) * 0.5f) * 48.f };
				if (!created || !history.Execute(std::make_unique<SetPropertyCommand>(created->GetUniqueID(), std::nullopt, "Location", location)))
				{
					success = false;
					break;
				}
			}
			break;
		}
		case ScriptOperation::Delete:
			success = DeleteSelection(context, history);
			break;
		case ScriptOperation::SelectAdd:
		case ScriptOperation::SelectRemove:
		{
			std::shared_ptr<ly::Actor> target;
			unsigned int id = 0;
			const auto number = std::from_chars(command.text.data(), command.text.data() + command.text.size(), id);
			for (const auto& weak : context.world->GetActorsByTypeIncludingPending<ly::Actor>())
			{
				const auto candidate = weak.lock();
				if (!candidate || candidate->GetIsPendingDestroy()) continue;
				const auto* identity = candidate->FindComponent<ly::SceneEntityComponent>();
				if ((identity && (identity->name == command.text || identity->guid == command.text)) ||
					(number.ec == std::errc{} && number.ptr == command.text.data() + command.text.size() && candidate->GetUniqueID() == id))
				{
					target = candidate;
					break;
				}
			}
			if (target)
			{
				if (command.operation == ScriptOperation::SelectAdd) context.selection.Add(target);
				else context.selection.Remove(target);
				success = true;
			}
			break;
		}
		case ScriptOperation::SelectAll:
			context.selection.Clear();
			for (const auto& weak : context.world->GetActorsByTypeIncludingPending<ly::Actor>()) context.selection.Add(weak);
			success = true;
			break;
		case ScriptOperation::SelectRect:
			SelectRectangle(context, command.location, command.rectangleEnd);
			success = true;
			break;
		case ScriptOperation::ExpectSelectionCount:
			success = context.selection.Count() == command.number;
			detail = "selection count " + std::to_string(context.selection.Count()) + " expected " + std::to_string(command.number);
			break;
		case ScriptOperation::MoveSelection:
			success = TransformSelection(context, history, CaptureSelectionTransform(context.selection), context.selection.Centre(), command.location);
			break;
		case ScriptOperation::Copy: success = CopySelection(context); break;
		case ScriptOperation::Paste:
			success = PasteSelection(context, history, command.number ? command.location : context.world->GetWorldView().getCenter());
			break;
		case ScriptOperation::SetPropAll:
		{
			MultiInspectorCache cache;
			const auto& rows = cache.Get(context);
			const CommonProperty* match = nullptr;
			bool ambiguous = false;
			for (const auto& row : rows) if (row.property->name == command.property)
			{
				if (match) ambiguous = true;
				match = &row;
			}
			ly::reflect::PropertyValue value;
			float tolerance = command.tolerance;
			if (!match || ambiguous) detail = "Unknown or ambiguous common property";
			else if (!ParsePropertyValue(*match->property, command.values, false, value, tolerance)) detail = "Invalid common property value";
			else success = SetCommonProperty(context, history, *match, value);
			break;
		}
		case ScriptOperation::Duplicate: success = DuplicateSceneEntity(context, history); break;
		case ScriptOperation::Rename: success = RenameSceneEntity(context, history, command.text); break;
		case ScriptOperation::SelectName:
		case ScriptOperation::ExpectEntityCount:
		{
			unsigned int count = 0;
			for (const auto& weak : context.world->GetActorsByTypeIncludingPending<ly::Actor>())
			{
				const auto candidate = weak.lock();
				const auto* identity = candidate ? candidate->FindComponent<ly::SceneEntityComponent>() : nullptr;
				if (!identity || candidate->GetIsPendingDestroy()) continue;
				++count;
				if (command.operation == ScriptOperation::SelectName && identity->name == command.text)
				{
					context.selection.Select(candidate);
					success = true;
					break;
				}
			}
			if (command.operation == ScriptOperation::ExpectEntityCount)
			{
				success = count == command.number;
				detail = "count " + std::to_string(count) + " expected " + std::to_string(command.number);
			}
			break;
		}
		case ScriptOperation::ExpectMode: success = context.playMode == (command.text == "play"); break;
		case ScriptOperation::ExpectSceneDirty: success = context.sceneDirty && context.sceneDirty() == (command.text == "true"); break;
		case ScriptOperation::AddComponent:
			if (actor) success = history.Execute(std::make_unique<AddComponentCommand>(actor->GetUniqueID(), command.text));
			break;
		case ScriptOperation::RemoveComponent:
			if (actor) success = history.Execute(std::make_unique<RemoveComponentCommand>(actor->GetUniqueID(), ly::ComponentIdentity{ command.text, command.number }));
			break;
		case ScriptOperation::ExpectComponent:
			if (actor) success = (ly::FindComponentByTypeOccurrence(*actor, { command.text, 0 }) != nullptr) == (command.property == "present");
			break;
		case ScriptOperation::AddScript:
			if (actor && !context.playMode)
			{
				auto& script = actor->AddComponent<script::ScriptComponent>(command.text);
				context.SceneChanged();
				success = script.Prepare();
				detail = success ? "Script attached (not undoable)" : script.LastError();
			}
			break;
		case ScriptOperation::ExpectScriptError:
			if (auto* script = actor ? actor->FindComponent<script::ScriptComponent>() : nullptr)
			{
				success = command.text == "none" ? script->LastError().empty() : script->LastError().find(command.text) != std::string::npos;
				detail = script->LastError();
			}
			break;
		case ScriptOperation::SetScriptProp:
		case ScriptOperation::ExpectScriptProp:
		{
			const auto target = actor ? FindPropertyTarget(*actor, "ScriptComponent") : ReflectedTarget{};
			auto* script = dynamic_cast<script::ScriptComponent*>(static_cast<ly::ActorComponent*>(target.object));
			if (!script) { detail = "No ScriptComponent"; break; }
			const auto entry = script->Properties().find(command.property);
			if (entry == script->Properties().end()) { detail = "Unknown script property"; break; }
			ly::reflect::PropertyInfo property;
			property.kind = ly::reflect::KindOf(entry->second);
			ly::reflect::PropertyValue value;
			float tolerance = command.tolerance;
			const bool expect = command.operation == ScriptOperation::ExpectScriptProp;
			if (!ParsePropertyValue(property, command.values, expect, value, tolerance)) detail = "Invalid script property value or tolerance";
			else if (expect) success = PropertyEquals(entry->second, value, tolerance);
			else success = history.Execute(std::make_unique<SetScriptPropertyCommand>(actor->GetUniqueID(), *target.componentIdentity, command.property, value));
			break;
		}
		case ScriptOperation::WaitSeconds:
			mWaitUntil = std::chrono::steady_clock::now() + std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<float>{ command.factor });
			success = true;
			break;
		case ScriptOperation::RememberRotation:
			if (actor) { mRememberedRotation = actor->GetActorRotation(); success = true; }
			break;
		case ScriptOperation::ExpectRotationChanged:
			if (actor && mRememberedRotation)
			{
				const float rotation = actor->GetActorRotation();
				success = std::abs(std::remainder(rotation - *mRememberedRotation, 360.f)) > 0.01f;
				detail = "rotation " + std::to_string(rotation) + " before " + std::to_string(*mRememberedRotation);
			}
			break;
		case ScriptOperation::OpenJson:
		case ScriptOperation::ContentOpen:
		case ScriptOperation::ContentSet:
		case ScriptOperation::ContentNew:
		case ScriptOperation::ContentRename:
		case ScriptOperation::ContentSave:
		case ScriptOperation::ContentValidate:
		case ScriptOperation::ContentReload:
		case ScriptOperation::ExpectContent:
			if (command.operation != ScriptOperation::OpenJson)
			{
				success = context.contentCommand && context.contentCommand(command, detail);
				break;
			}
			if (context.document && !context.document->Dirty()) success = context.document->Open(command.text);
			detail = context.document && context.document->Dirty() ? "Unsaved changes: save or revert before open-json" :
				context.document ? context.document->Error() : "No JSON document service";
			break;
		case ScriptOperation::JsonSet:
		case ScriptOperation::ExpectJson:
		{
			const auto* actual = context.document ? context.document->Get(command.text) : nullptr;
			const auto expected = Json::parse(command.values[0]);
			if (actual)
			{
				if (command.operation == ScriptOperation::JsonSet)
					success = history.Execute(std::make_unique<SetJsonValueCommand>(context.document->ID(), command.text, *actual, expected));
				else success = *actual == expected && (actual->type() == expected.type() ||
					(actual->is_number_integer() && expected.is_number_integer()));
				detail = "actual " + actual->dump() + " expected " + expected.dump();
			}
			else detail = "Unknown JSON pointer or no open document";
			break;
		}
		case ScriptOperation::JsonSave:
			success = !context.playMode && context.document && context.document->Save();
			if (context.document) detail = context.document->Error();
			break;
		case ScriptOperation::JsonRevert:
			success = !context.playMode && context.document && context.document->Revert();
			if (context.document) detail = context.document->Error();
			break;
		case ScriptOperation::ExpectJsonDirty:
			success = context.document && context.document->IsOpen() && context.document->Dirty() == (command.text == "true");
			break;
		case ScriptOperation::ExpectPropError:
			success = mPendingPropertyError && mPendingPropertyError->find(command.text) != std::string::npos;
			detail = mPendingPropertyError.value_or("No failed property write to handle");
			mPendingPropertyError.reset();
			break;
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
				success = history.Execute(std::make_unique<SetPropertyCommand>(actor->GetUniqueID(), target.componentIdentity, property->name, value));
				detail = success ? "Property set" : context.propertyError;
				if (!success) mPendingPropertyError = detail;
			}
			break;
		}
		case ScriptOperation::Tool: context.viewSettings.tool = command.text == "move" ? EditorTool::Move : EditorTool::Rotate; success = true; break;
		case ScriptOperation::Snap: context.viewSettings.snap = command.text == "on"; success = true; break;
		case ScriptOperation::Grid: context.viewSettings.grid = command.text == "on"; success = true; break;
		case ScriptOperation::Colliders: context.viewSettings.colliders = command.text == "on"; success = true; break;
		case ScriptOperation::ContentView:
			context.viewSettings.contentGrid = command.text == "grid";
			context.viewSettings.requestContent = true;
			success = true;
			break;
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
			for (const auto& weak : context.world->GetActorsByTypeIncludingPending<ly::Actor>())
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
			if (actor) success = history.Execute(std::make_unique<SetPropertyCommand>(actor->GetUniqueID(), std::nullopt, "Location", command.location));
			break;
		case ScriptOperation::Rotate:
			if (actor) success = history.Execute(std::make_unique<SetPropertyCommand>(actor->GetUniqueID(), std::nullopt, "Rotation", command.degrees));
			break;
		case ScriptOperation::Undo: success = context.extensions && context.extensions->History() ? context.extensions->History()->Undo() : history.Undo(); break;
		case ScriptOperation::Redo: success = context.extensions && context.extensions->History() ? context.extensions->History()->Redo() : history.Redo(); break;
		case ScriptOperation::ExpectSelected:
			success = actor && ActorTypeName(*actor) == command.text;
			detail = "selected " + (actor ? ActorTypeName(*actor) : std::string{ "none" });
			break;
		case ScriptOperation::ExpectLocation: success = expectLocation(command.location); break;
		case ScriptOperation::ExpectCount:
		{
			unsigned int count = 0;
			for (const auto& weak : context.world->GetActorsByTypeIncludingPending<ly::Actor>())
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
		case ScriptOperation::SetProjectStartup:
		case ScriptOperation::ExpectProjectStartup:
		{
			try
			{
				if (!context.document) throw std::runtime_error("No project asset root");
				const auto path = context.document->Root() / "vega.project.json";
				auto project = std::filesystem::exists(path) ? ly::ProjectSettings::Load(path) : ly::ProjectSettings{};
				if (command.operation == ScriptOperation::ExpectProjectStartup) success = project.startupScene == command.text;
				else
				{
					project.startupScene = command.text;
					project = ly::ProjectSettings::Parse(project.ToJson());
					success = AtomicWriteText(path, project.ToJson().dump(2) + "\n", detail);
				}
			}
			catch (const std::exception& error) { detail = error.what(); }
			break;
		}
		case ScriptOperation::Export:
		{
			// The script always runs the smoke test, so expect-export-ok can check the whole result.
			GameExportRequest request;
			request.outputDir = std::filesystem::u8path(command.text);
			request.overwrite = command.overwrite;
			request.runSmokeTest = true;
			try
			{
				mExportFuture = ExportGameAsync(mExportLayout, request);
				mExportLine = line;
				return false;
			}
			catch (const std::exception& error) { detail = error.what(); }
			break;
		}
		case ScriptOperation::ExpectExportOk:
			success = mLastExport && mLastExport->success && mLastExport->smokeTestRan && mLastExport->smokeTestPassed;
			detail = mLastExport ? DescribeGameExport(*mLastExport) : std::string{ "No export has run" };
			break;
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
		Report(line, success, detail, mPendingPropertyError.has_value());
		return mFinished;
	}
}
