#include "editor/EditorScript.h"
#include "editor/ContentPath.h"
#include "editor/SkyHandle.h"
#include <charconv>
#include <cmath>
#include <locale>
#include <sstream>
#include <cctype>
#include <nlohmann/json.hpp>

namespace vega
{
	EditorScriptParseResult ParseEditorScriptLine(const std::string& line, const std::function<bool(const std::string&)>& isExtension)
	{
		std::vector<std::string> tokens;
		std::vector<std::string> rawTokens;
		// Comments only start outside quotes. Quoted arguments preserve spaces,
		// hashes and empty strings; backslashes are literal except before a quote or backslash.
		std::size_t cursor = 0;
		while (cursor < line.size())
		{
			if (std::isspace(static_cast<unsigned char>(line[cursor]))) { ++cursor; continue; }
			if (line[cursor] == '#') break;
			const auto start = cursor;
			std::string token;
			if (line[cursor] == '"')
			{
				++cursor;
				bool closed = false;
				while (cursor < line.size())
				{
					char character = line[cursor++];
					if (character == '"') { closed = true; break; }
					if (character == '\\' && cursor < line.size() && (line[cursor] == '"' || line[cursor] == '\\')) character = line[cursor++];
					token += character;
				}
				if (!closed || (cursor < line.size() && !std::isspace(static_cast<unsigned char>(line[cursor])) && line[cursor] != '#'))
					return { {}, "Invalid quoted argument" };
			}
			else if (line[cursor] == '[' || line[cursor] == '{')
			{
				// Composite JSON is one argument, including spaces and quoted keys.
				// JSON parsing below checks matching delimiters and scalar validity.
				int depth = 0;
				bool quoted = false, escaped = false;
				do
				{
					const char c = line[cursor++];
					token += c;
					if (quoted)
					{
						if (escaped) escaped = false;
						else if (c == '\\') escaped = true;
						else if (c == '"') quoted = false;
					}
					else if (c == '"') quoted = true;
					else if (c == '[' || c == '{') ++depth;
					else if (c == ']' || c == '}') --depth;
				} while (cursor < line.size() && (depth > 0 || quoted));
				if (depth != 0 || quoted || (cursor < line.size() && !std::isspace(static_cast<unsigned char>(line[cursor])) && line[cursor] != '#')) return { {}, "Invalid JSON argument" };
			}
			else
			{
				while (cursor < line.size() && !std::isspace(static_cast<unsigned char>(line[cursor])) && line[cursor] != '#')
				{
					if (line[cursor] == '"') return { {}, "Quote must start an argument" };
					token += line[cursor++];
				}
			}
			tokens.push_back(std::move(token));
			rawTokens.push_back(line.substr(start, cursor - start));
		}
		if (tokens.empty()) return {};
		EditorScriptCommand command;
		const auto bad = []() { return EditorScriptParseResult{ {}, "Invalid arguments" }; };
		const auto integer = [&tokens](std::size_t index, unsigned int& value)
		{
			const auto& token = tokens[index];
			const auto result = std::from_chars(token.data(), token.data() + token.size(), value);
			return result.ec == std::errc{} && result.ptr == token.data() + token.size();
		};
		const auto real = [&tokens](std::size_t index, float& value)
		{
			std::istringstream input{ tokens[index] };
			input.imbue(std::locale::classic());
			return (input >> value) && input.eof() && std::isfinite(value);
		};
		const std::string& verb = tokens[0];
		command.extensionName = verb;
		if (verb == "sky-open" || verb == "capture-sky" || verb == "sky-save")
		{
			if (tokens.size() != (verb == "sky-save" ? 1u : 2u) || (tokens.size() == 2 && tokens[1].empty())) return bad();
			command.operation = verb == "sky-open" ? ScriptOperation::SkyOpen : verb == "sky-save" ? ScriptOperation::SkySave : ScriptOperation::CaptureSky;
			if (tokens.size() == 2) command.text = tokens[1];
			return {command, {}};
		}
		if (verb == "sky-set" || verb == "expect-sky")
		{
			if (tokens.size() != 3) return bad();
			try
			{
				ContentPointer(tokens[1]);
				const auto value = nlohmann::json::parse(rawTokens[2]);
				if (value.is_null() || value.is_object()) return bad();
			}
			catch (const std::exception&) { return bad(); }
			command.operation = verb == "sky-set" ? ScriptOperation::SkySet : ScriptOperation::ExpectSky;
			command.text = tokens[1]; command.values = {rawTokens[2]}; return {command, {}};
		}
		if (verb == "sky-drag")
		{
			if (tokens.size() != 4 || !IsSkyHandle(tokens[1]) || !real(2, command.location.x) || !real(3, command.location.y)) return bad();
			command.operation = ScriptOperation::SkyDrag; command.text = tokens[1]; return {command, {}};
		}
		if (verb == "content-validate" || verb == "content-save" || verb == "content-reload")
		{
			if (tokens.size() != 1) return bad();
			command.operation = verb == "content-validate" ? ScriptOperation::ContentValidate : verb == "content-save" ? ScriptOperation::ContentSave : ScriptOperation::ContentReload;
			return { command, {} };
		}
		if (verb == "content-open" || verb == "content-new" || verb == "content-rename" || verb == "expect-content" || verb == "content-set")
		{
			const std::size_t arity = verb == "expect-content" ? 5 : verb == "content-new" || verb == "content-rename" ? 4 : 3;
			if (tokens.size() != arity) return bad();
			for (std::size_t i = 1; i < tokens.size(); ++i) if (tokens[i].empty() && !(verb == "content-set" && i == 2) && !(verb == "expect-content" && i == 4)) return bad();
			if (verb != "content-set")
				for (unsigned char c : tokens[1]) if (!std::isalnum(c) && c != '_' && c != '-') return bad();
			if (verb == "content-set" || verb == "expect-content")
			{
				const std::size_t index = verb == "content-set" ? 1 : 3;
				try
				{
					ContentPointer(tokens[index]);
					const auto value = nlohmann::ordered_json::parse(rawTokens[index + 1]);
					if (value.is_null()) return bad();
				}
				catch (const std::exception&) { return bad(); }
				command.operation = verb == "content-set" ? ScriptOperation::ContentSet : ScriptOperation::ExpectContent;
				command.text = tokens[1];
				if (verb == "expect-content") { command.property = tokens[2]; command.values = { tokens[3], rawTokens[4] }; }
				else command.values = { rawTokens[2] };
			}
			else
			{
				command.operation = verb == "content-open" ? ScriptOperation::ContentOpen : verb == "content-new" ? ScriptOperation::ContentNew : ScriptOperation::ContentRename;
				command.text = tokens[1]; command.property = tokens[2];
				if (tokens.size() == 4) command.values = { tokens[3] };
			}
			return { command, {} };
		}
		if (verb == "gas-filter")
		{
			if (tokens.size() != 3 || (tokens[1] != "abilities" && tokens[1] != "effects" && tokens[1] != "attributes" && tokens[1] != "tags" && tokens[1] != "events")) return bad();
			command.operation = ScriptOperation::GasFilter;
			command.text = tokens[1];
			command.property = tokens[2];
			return { command, {} };
		}
		if (verb == "gas-select-player" || verb == "gas-reset-cooldowns")
		{
			if (tokens.size() != 1) return bad();
			command.operation = verb == "gas-select-player" ? ScriptOperation::GasSelectPlayer : ScriptOperation::GasResetCooldowns;
			return { command, {} };
		}
		if (verb == "gas-apply-effect" || verb == "gas-remove-effect" || verb == "gas-activate" || verb == "gas-panel")
		{
			if (tokens.size() != 2 || tokens[1].empty()) return bad();
			if (verb == "gas-panel" && tokens[1] != "on" && tokens[1] != "off") return bad();
			command.operation = verb == "gas-apply-effect" ? ScriptOperation::GasApplyEffect : verb == "gas-remove-effect" ? ScriptOperation::GasRemoveEffect : verb == "gas-activate" ? ScriptOperation::GasActivate : ScriptOperation::GasPanel;
			command.text = tokens[1];
			return { command, {} };
		}
		if (verb == "expect-ability" || verb == "expect-effect" || verb == "expect-tag" || verb == "expect-attribute")
		{
			if (tokens.size() != (verb == "expect-attribute" ? 4u : 3u) || tokens[1].empty()) return bad();
			command.text = tokens[1];
			command.property = tokens[2];
			if (verb == "expect-attribute")
			{
				if ((tokens[2] != "==" && tokens[2] != "<" && tokens[2] != ">" && tokens[2] != "<=" && tokens[2] != ">=") || !real(3, command.factor)) return bad();
				command.operation = ScriptOperation::ExpectAttribute;
			}
			else if (verb == "expect-ability")
			{
				if (tokens[2] != "ready" && tokens[2] != "cooldown" && tokens[2] != "active") return bad();
				command.operation = ScriptOperation::ExpectAbility;
			}
			else
			{
				if (tokens[2] != "true" && tokens[2] != "false") return bad();
				command.operation = verb == "expect-effect" ? ScriptOperation::ExpectEffect : ScriptOperation::ExpectTag;
			}
			return { command, {} };
		}
		if (verb == "select-add" || verb == "select-remove")
		{
			if (tokens.size() != 2 || tokens[1].empty()) return bad();
			command.operation = verb == "select-add" ? ScriptOperation::SelectAdd : ScriptOperation::SelectRemove;
			command.text = tokens[1];
			return { command, {} };
		}
		if (verb == "select-all" || verb == "copy")
		{
			if (tokens.size() != 1) return bad();
			command.operation = verb == "copy" ? ScriptOperation::Copy : ScriptOperation::SelectAll;
			return { command, {} };
		}
		if (verb == "expect-selection-count")
		{
			if (tokens.size() != 2 || !integer(1, command.number)) return bad();
			command.operation = ScriptOperation::ExpectSelectionCount;
			return { command, {} };
		}
		if (verb == "select-rect")
		{
			if (tokens.size() != 5 || !real(1, command.location.x) || !real(2, command.location.y) ||
				!real(3, command.rectangleEnd.x) || !real(4, command.rectangleEnd.y)) return bad();
			command.operation = ScriptOperation::SelectRect;
			return { command, {} };
		}
		if (verb == "move-selection" || verb == "paste")
		{
			if (verb == "paste" && tokens.size() == 1) command.number = 0;
			else
			{
				if (tokens.size() != 3 || !real(1, command.location.x) || !real(2, command.location.y)) return bad();
				command.number = 1;
			}
			command.operation = verb == "paste" ? ScriptOperation::Paste : ScriptOperation::MoveSelection;
			return { command, {} };
		}
		if (verb == "set-prop-all")
		{
			if (tokens.size() < 3 || tokens[1].empty()) return bad();
			command.operation = ScriptOperation::SetPropAll;
			command.property = tokens[1];
			command.values.assign(tokens.begin() + 2, tokens.end());
			return { command, {} };
		}
		if (verb == "set-setting" || verb == "expect-setting" || verb == "reset-setting")
		{
			const bool reset = verb == "reset-setting";
			if (tokens.size() != (reset ? 2u : 3u) || tokens[1].empty()) return bad();
			command.operation = reset ? ScriptOperation::ResetSetting : verb == "set-setting" ? ScriptOperation::SetSetting : ScriptOperation::ExpectSetting;
			command.text = tokens[1];
			if (!reset)
			{
				try
				{
					const auto value = nlohmann::json::parse(rawTokens[2]);
					if (value.is_null() || value.is_object()) return bad();
				}
				catch (const std::exception&) { return bad(); }
				command.values = { rawTokens[2] };
			}
			return { command, {} };
		}
		if (verb == "expect-backdrop")
		{
			if (tokens.size() != 2 || tokens[1].empty()) return bad();
			command.operation = ScriptOperation::ExpectBackdrop;
			command.text = tokens[1];
			return { command, {} };
		}
		if (verb == "expect-console")
		{
			if (tokens.size() != 3 || tokens[2].empty() || (tokens[1] != "Info" && tokens[1] != "Warning" && tokens[1] != "Error")) return bad();
			command.operation = ScriptOperation::ExpectConsole;
			command.property = tokens[1];
			command.text = tokens[2];
			return { command, {} };
		}
		if (verb == "expect-no-errors" || verb == "clear-console")
		{
			if (tokens.size() != 1) return bad();
			command.operation = verb == "expect-no-errors" ? ScriptOperation::ExpectNoErrors : ScriptOperation::ClearConsole;
			return { command, {} };
		}
		if (verb == "profiler" || verb == "diagnostics" || verb == "expect-profile-scope" || verb == "capture-profile")
		{
			if (tokens.size() != 2 || tokens[1].empty()) return bad();
			if ((verb == "profiler" || verb == "diagnostics") && tokens[1] != "on" && tokens[1] != "off") return bad();
			command.operation = verb == "profiler" ? ScriptOperation::Profiler : verb == "diagnostics" ? ScriptOperation::Diagnostics :
				verb == "expect-profile-scope" ? ScriptOperation::ExpectProfileScope : ScriptOperation::CaptureProfile;
			command.text = tokens[1];
			return { command, {} };
		}
		if (verb == "open-prefab" || verb == "save-prefab" || verb == "close-prefab")
		{
			if (tokens.size() != (verb == "open-prefab" ? 2u : 1u)) return bad();
			if (verb == "open-prefab")
			{
				if (tokens[1].empty()) return bad();
				command.text = tokens[1];
			}
			command.operation = verb == "open-prefab" ? ScriptOperation::OpenPrefab : verb == "save-prefab" ? ScriptOperation::SavePrefab : ScriptOperation::ClosePrefab;
			return { command, {} };
		}
		if (verb == "create-prefab" || verb == "expect-prefab" || verb == "reload-prefabs" || verb == "place-prefab" || verb == "create-variant" ||
			verb == "apply-prefab-prop" || verb == "revert-prefab-prop" || verb == "expect-overridden")
		{
			if (verb == "reload-prefabs")
			{
				if (tokens.size() != 1) return bad();
				command.operation = ScriptOperation::ReloadPrefabs;
			}
			else
			{
				if (tokens.size() < 2 || tokens[1].empty()) return bad();
				command.text = tokens[1];
				if (verb == "place-prefab")
				{
					if (tokens.size() != 2 && tokens.size() != 4) return bad();
					if (tokens.size() == 4 && (!real(2, command.location.x) || !real(3, command.location.y))) return bad();
					command.number = tokens.size() == 4 ? 1 : 0;
					command.operation = ScriptOperation::PlacePrefab;
				}
				else if (verb == "create-prefab" || verb == "expect-prefab")
				{
					if (tokens.size() != 2) return bad();
					command.operation = verb == "create-prefab" ? ScriptOperation::CreatePrefab : ScriptOperation::ExpectPrefab;
				}
				else
				{
					if (tokens.size() != (verb == "expect-overridden" ? 4u : 3u) || tokens[2].empty()) return bad();
					command.property = tokens[2];
					if (verb == "expect-overridden")
					{
						if (tokens[3] != "true" && tokens[3] != "false") return bad();
						command.overwrite = tokens[3] == "true";
						command.operation = ScriptOperation::ExpectOverridden;
					}
					else command.operation = verb == "create-variant" ? ScriptOperation::CreateVariant : verb == "apply-prefab-prop" ? ScriptOperation::ApplyPrefabProp : ScriptOperation::RevertPrefabProp;
				}
			}
			return { command, {} };
		}
		if (verb == "add-component" || verb == "remove-component" || verb == "expect-component")
		{
			if (tokens.size() < 2 || tokens[1].empty()) return bad();
			command.text = tokens[1];
			if (verb == "add-component")
			{
				if (tokens.size() != 2) return bad();
				command.operation = ScriptOperation::AddComponent;
			}
			else if (verb == "remove-component")
			{
				if (tokens.size() > 3 || (tokens.size() == 3 && !integer(2, command.number))) return bad();
				command.operation = ScriptOperation::RemoveComponent;
			}
			else
			{
				if (tokens.size() != 3 || (tokens[2] != "present" && tokens[2] != "absent")) return bad();
				command.operation = ScriptOperation::ExpectComponent;
				command.property = tokens[2];
			}
			return { command, {} };
		}
		if (verb == "create-many")
		{
			if (tokens.size() != 3 || tokens[1].empty() || !integer(2, command.number) || command.number > 10000) return bad();
			command.operation = ScriptOperation::CreateMany;
			command.text = tokens[1];
			return { command, {} };
		}
		if (verb == "expect-prop-error")
		{
			if (tokens.size() != 2 || tokens[1].empty()) return bad();
			command.operation = ScriptOperation::ExpectPropError;
			command.text = tokens[1];
			return { command, {} };
		}
		if (verb == "rename" || verb == "select-name")
		{
			if (tokens.size() < 2) return bad();
			for (std::size_t index = 1; index < tokens.size(); ++index)
			{
				if (index > 1) command.text += ' ';
				command.text += tokens[index];
			}
			if (command.text.empty()) return bad();
			command.operation = verb == "rename" ? ScriptOperation::Rename : ScriptOperation::SelectName;
			return { command, {} };
		}
		if (verb == "set-project-startup" || verb == "expect-project-startup")
		{
			if (tokens.size() != 2) return bad();
			command.operation = verb == "set-project-startup" ? ScriptOperation::SetProjectStartup : ScriptOperation::ExpectProjectStartup;
			command.text = tokens[1];
			return { command, {} };
		}
		if (verb == "open-scene" || verb == "save-scene" || verb == "create" || verb == "expect-mode" || verb == "expect-scene-dirty")
		{
			if (verb == "save-scene" && tokens.size() == 1)
			{
				command.operation = ScriptOperation::SaveScene;
				return { command, {} };
			}
			if (tokens.size() != 2 || tokens[1].empty()) return bad();
			command.text = tokens[1];
			if (verb == "expect-mode" && command.text != "edit" && command.text != "play") return bad();
			if (verb == "expect-scene-dirty" && command.text != "true" && command.text != "false") return bad();
			command.operation = verb == "open-scene" ? ScriptOperation::OpenScene : verb == "save-scene" ? ScriptOperation::SaveScene :
				verb == "create" ? ScriptOperation::Create : verb == "expect-mode" ? ScriptOperation::ExpectMode : ScriptOperation::ExpectSceneDirty;
			return { command, {} };
		}
		if (verb == "expect-entity-count")
		{
			if (tokens.size() != 2 || !integer(1, command.number)) return bad();
			command.operation = ScriptOperation::ExpectEntityCount;
			return { command, {} };
		}
		if (verb == "new-scene" || verb == "delete" || verb == "duplicate" || verb == "play-mode" || verb == "stop")
		{
			if (tokens.size() != 1) return bad();
			command.operation = verb == "new-scene" ? ScriptOperation::NewScene : verb == "delete" ? ScriptOperation::Delete :
				verb == "duplicate" ? ScriptOperation::Duplicate : verb == "play-mode" ? ScriptOperation::PlayMode : ScriptOperation::Stop;
			return { command, {} };
		}
		if (verb == "tool" || verb == "snap" || verb == "grid" || verb == "colliders" || verb == "content-view")
		{
			if (tokens.size() != 2) return bad();
			if (verb == "tool")
			{
				if (tokens[1] != "move" && tokens[1] != "rotate") return bad();
				command.operation = ScriptOperation::Tool;
			}
			else if (verb == "content-view")
			{
				if (tokens[1] != "grid" && tokens[1] != "list") return bad();
				command.operation = ScriptOperation::ContentView;
			}
			else
			{
				if (tokens[1] != "on" && tokens[1] != "off") return bad();
				command.operation = verb == "snap" ? ScriptOperation::Snap : verb == "grid" ? ScriptOperation::Grid : ScriptOperation::Colliders;
			}
			command.text = tokens[1];
			return { command, {} };
		}
		if (verb == "add-script" || verb == "expect-script-error")
		{
			if (tokens.size() != 2 || tokens[1].empty()) return bad();
			command.operation = verb == "add-script" ? ScriptOperation::AddScript : ScriptOperation::ExpectScriptError;
			command.text = tokens[1];
			return { command, {} };
		}
		if (verb == "set-script-prop" || verb == "expect-script-prop")
		{
			if (tokens.size() < 3 || tokens[1].empty()) return bad();
			command.operation = verb == "set-script-prop" ? ScriptOperation::SetScriptProp : ScriptOperation::ExpectScriptProp;
			command.property = tokens[1];
			command.values.assign(tokens.begin() + 2, tokens.end());
			return { command, {} };
		}
		if (verb == "wait-seconds")
		{
			if (tokens.size() != 2 || !real(1, command.factor) || command.factor < 0.f || command.factor > 86400.f) return bad();
			command.operation = ScriptOperation::WaitSeconds;
			return { command, {} };
		}
		if (verb == "open-json")
		{
			if (tokens.size() != 2 || tokens[1].empty()) return bad();
			command.operation = ScriptOperation::OpenJson;
			command.text = tokens[1];
			return { command, {} };
		}
		else if (verb == "json-set" || verb == "expect-json")
		{
			if (tokens.size() != 3) return bad();
			try
			{
				const nlohmann::ordered_json::json_pointer pointer{ tokens[1] };
				const auto value = nlohmann::ordered_json::parse(rawTokens[2]);
				if (value.is_structured() || value.is_null() || (value.is_number_float() && !std::isfinite(value.get<double>()))) return bad();
			}
			catch (const std::exception&) { return bad(); }
			command.operation = verb == "json-set" ? ScriptOperation::JsonSet : ScriptOperation::ExpectJson;
			command.text = tokens[1];
			command.values = { rawTokens[2] };
			return { command, {} };
		}
		else if (verb == "expect-json-dirty")
		{
			if (tokens.size() != 2 || (tokens[1] != "true" && tokens[1] != "false")) return bad();
			command.operation = ScriptOperation::ExpectJsonDirty;
			command.text = tokens[1];
			return { command, {} };
		}
		else if (verb == "export")
		{
			if (tokens.size() < 2 || tokens.size() > 3 || tokens[1].empty() ||
				(tokens.size() == 3 && tokens[2] != "overwrite")) return bad();
			command.operation = ScriptOperation::Export;
			command.text = tokens[1];
			command.overwrite = tokens.size() == 3;
			return { command, {} };
		}
		else if (verb == "expect-export-ok") command.operation = ScriptOperation::ExpectExportOk;
		else if (verb == "json-save") command.operation = ScriptOperation::JsonSave;
		else if (verb == "json-revert") command.operation = ScriptOperation::JsonRevert;
		else if (verb == "set-prop" || verb == "expect-prop")
		{
			if (tokens.size() < 4 || tokens[1].empty() || tokens[2].empty()) return bad();
			command.operation = verb == "set-prop" ? ScriptOperation::SetProp : ScriptOperation::ExpectProp;
			command.text = tokens[1];
			command.property = tokens[2];
			command.values.assign(tokens.begin() + 3, tokens.end());
			// Arity and scalar types depend on the selected object's descriptor.
			return { command, {} };
		}
		else if (verb == "pause") command.operation = ScriptOperation::Pause;
		else if (verb == "play") command.operation = ScriptOperation::Play;
		else if (verb == "focus") command.operation = ScriptOperation::Focus;
		else if (verb == "expect-visible") command.operation = ScriptOperation::ExpectVisible;
		else if (verb == "camera" || verb == "expect-camera")
		{
			if (tokens.size() != 2 || (tokens[1] != "editor" && tokens[1] != "game")) return bad();
			command.text = tokens[1];
			command.operation = verb == "camera" ? ScriptOperation::Camera : ScriptOperation::ExpectCamera;
			return { command, {} };
		}
		else if (verb == "zoom")
		{
			if (tokens.size() != 2 || !real(1, command.factor) || command.factor <= 0.f) return bad();
			command.operation = ScriptOperation::Zoom;
			return { command, {} };
		}
		else if (verb == "pan")
		{
			if (tokens.size() != 3 || !real(1, command.location.x) || !real(2, command.location.y)) return bad();
			command.operation = ScriptOperation::Pan;
			return { command, {} };
		}
		else if (verb == "undo") command.operation = ScriptOperation::Undo;
		else if (verb == "redo") command.operation = ScriptOperation::Redo;
		else if (verb == "quit") command.operation = ScriptOperation::Quit;
		else if (verb == "remember-location") command.operation = ScriptOperation::RememberLocation;
		else if (verb == "remember-rotation") command.operation = ScriptOperation::RememberRotation;
		else if (verb == "expect-rotation-changed") command.operation = ScriptOperation::ExpectRotationChanged;
		else if (verb == "expect-remembered-location") command.operation = ScriptOperation::ExpectRememberedLocation;
		else if (verb == "wait" || verb == "select")
		{
			if (tokens.size() != 2 || !integer(1, command.number)) return bad();
			command.operation = verb == "wait" ? ScriptOperation::Wait : ScriptOperation::Select;
			return { command, {} };
		}
		else if (verb == "select-type")
		{
			if (tokens.size() < 2 || tokens.size() > 3 || (tokens.size() == 3 && !integer(2, command.number))) return bad();
			command.operation = ScriptOperation::SelectType;
			command.text = tokens[1];
			return { command, {} };
		}
		else if (verb == "move" || verb == "expect-location")
		{
			const bool expect = verb == "expect-location";
			if (tokens.size() < 3 || tokens.size() > (expect ? 4u : 3u) ||
				!real(1, command.location.x) || !real(2, command.location.y) ||
				(tokens.size() == 4 && (!real(3, command.tolerance) || command.tolerance < 0.f))) return bad();
			command.operation = expect ? ScriptOperation::ExpectLocation : ScriptOperation::Move;
			return { command, {} };
		}
		else if (verb == "rotate")
		{
			if (tokens.size() != 2 || !real(1, command.degrees)) return bad();
			command.operation = ScriptOperation::Rotate;
			return { command, {} };
		}
		else if (verb == "expect-selected" || verb == "capture" || verb == "expect-count")
		{
			if (tokens.size() != (verb == "expect-count" ? 3u : 2u)) return bad();
			command.text = tokens[1];
			if (verb == "expect-count")
			{
				if (!integer(2, command.number)) return bad();
				command.operation = ScriptOperation::ExpectCount;
			}
			else command.operation = verb == "capture" ? ScriptOperation::Capture : ScriptOperation::ExpectSelected;
			return { command, {} };
		}
		else
		{
			if (!isExtension || !isExtension(verb)) return { {}, "Unknown command: " + verb };
			command.operation = ScriptOperation::Extension;
			command.extensionName = verb;
			command.values.assign(tokens.begin() + 1, tokens.end());
			return { command, {} };
		}
		if (tokens.size() != 1) return bad();
		return { command, {} };
	}
}
