#include "editor/EditorSkyEditor.h"
#include "editor/PropertyWidgets.h"
#include "editor/ContentPath.h"
#include <framework/World.h>
#include <framework/scene/SceneSettings.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui-SFML.h>
#include <algorithm>
#include <cmath>
#include <limits>

namespace vega
{
	namespace
	{
		constexpr float Rad = 0.01745329252f;
		double Milliseconds(std::chrono::steady_clock::time_point start)
		{
			return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
		}
		std::string BlockKind(const std::string& path) { return path.find("objects.") == 0 ? "object" : path; }
	}

	EditorSkyEditor::EditorSkyEditor(std::shared_ptr<sky::SkyEditorHost> host) : mPipeline{std::move(host)}
	{
		if (mPipeline) mDocument = std::make_unique<SkyDocument>(mPipeline->EditorAssets().Root(), mPipeline->DocumentFile());
	}

	bool EditorSkyEditor::Open(const std::string& stop, bool reopen)
	{
		if (!mPipeline || !mDocument) return false;
		const auto& stops = mPipeline->Skies();
		const auto it = std::find_if(stops.begin(), stops.end(), [&](const auto& entry) { return entry.id == stop; });
		if (it == stops.end()) { mDocument->error = "Unknown sky: " + stop; return false; }
		if (!mDocument->document.IsOpen()) { if (!mDocument->Open()) return false; }
		else if (reopen && !mDocument->document.Dirty() && !mDocument->Reopen()) return false;
		mSkyIndex = static_cast<std::size_t>(it - stops.begin());
		if (!mDocument->Select(stop)) return false;
		mRowsRevision = 0; mAppliedRevision = 0; mObservedRevision = 0;
		mDragHandle.clear();
		mRefreshGeometryRows = true;
		visible = mWasVisible = mMode = true;
		mPipeline->SelectSky(mSkyIndex);
		mPipeline->SetSkyAuthoring(true);
		ApplyPreview(true);
		return true;
	}

	void EditorSkyEditor::EndPreview(EditorContext& scene)
	{
		if (!mPreviewing || !mPipeline) return;
		mPipeline->EditorAssets().EndDeepSkyPreview();
		mPipeline->SetSkyAuthoring(false);
		if (scene.sceneSettings && scene.applySettings) scene.applySettings(*scene.sceneSettings);
		mPreviewing = false; mAppliedRevision = 0; mCompare = false; mDragHandle.clear();
	}

	void EditorSkyEditor::Update(EditorContext& scene, sf::Vector2u output, bool suspended)
	{
		mOutput = output; mSuspended = suspended;
		if (!mPipeline || !mDocument) return;
		if (!visible || suspended) { EndPreview(scene); return; }
		const auto sceneIndex = mPipeline->SceneSelection(scene);
		const auto& stops = mPipeline->Skies();
		if (stops.empty()) return;
		if (!mDocument->document.IsOpen() || mSceneSkyIndex != sceneIndex)
		{
			mSceneSkyIndex = sceneIndex;
			if (!Open(stops[std::min(sceneIndex, stops.size() - 1)].id)) return;
		}
		if (!mPreviewing || mPipeline->SelectedSky() != stops[mSkyIndex].id)
		{
			mPipeline->SelectSky(mSkyIndex); mPipeline->SetSkyAuthoring(true);
		}
		ApplyPreview();
		mGeometry = mPipeline->Geometry(output);
	}

	void EditorSkyEditor::ApplyPreview(bool force)
	{
		if (!mDocument || !mDocument->document.IsOpen()) return;
		const auto revision = mDocument->document.Revision();
		if (mObservedRevision != revision)
		{
			mObservedRevision = revision; mEditTime = std::chrono::steady_clock::now();
			if (!mDocument->Refresh()) return;
			CacheRows();
		}
		if (mAppliedRevision == revision && mAppliedCompare == mCompare && mPreviewing && !force) return;
		// Only orbit edits wait for idle; ordinary planet/object gestures appear on the next frame.
		const bool orbit = mEffective.contains("orbit");
		mRebuilding = !force && mPreviewing && orbit && mAppliedCompare == mCompare && Milliseconds(mEditTime) < 180.;
		if (mRebuilding) return;
		const auto start = std::chrono::steady_clock::now();
		if (mDocument->preset)
		{
			Json data = mCompare ? mDocument->Saved() : mDocument->document.Data();
			if (data.contains("sky")) data["sky"] = mDocument->selected;
			else data[data.contains("skies") ? "skies" : "stops"][mPipeline->Skies()[mSkyIndex].id] = mDocument->selected;
			if (!mPipeline->EditorAssets().PreviewDeepSky(nlohmann::json(data), mDocument->error)) return;
		}
		else mPipeline->EditorAssets().PreviewDeepSky(mCompare ? mDocument->SavedParsed() : mDocument->Parsed());
		mApplyMs = Milliseconds(start);
		mAppliedRevision = revision; mAppliedCompare = mCompare; mPreviewing = true; mRebuilding = false;
	}

	void EditorSkyEditor::CacheRows()
	{
		if (!mDocument) return;
		const std::string selection = (mDocument->preset ? "preset:" : "sky:") + mDocument->selected;
		if (mRowsRevision == mDocument->document.Revision() && mRowsSelection == selection) return;
		mEffective = mDocument->Effective(); mRows.clear(); mUnknown.clear();
		for (const auto& field : sky::DeepSkyFields())
		{
			if (field.path.find('*') != std::string::npos)
			{
				const auto end = field.path.find('.');
				const auto array = field.path.substr(0, end);
				if (!mEffective.contains(array) || !mEffective[array].is_array()) continue;
				for (std::size_t i = 0; i < mEffective[array].size(); ++i)
				{
					std::string path = field.path; path.replace(path.find('*'), 1, std::to_string(i));
					const auto value = mDocument->Value(path);
					mRows.push_back({&field, path, value, mDocument->Authored(path) != nullptr, sky::ValidateSkyField(field, nlohmann::json(value))});
				}
			}
			else
			{
				auto value = mDocument->Value(field.path);
				const Json::json_pointer pointer{ContentPointer(field.path)};
				if (!mEffective.contains(pointer))
				{
					// Optional parser fields inherit seeded composition or the region palette.
					// Show the current composition without materializing those defaults.
					if (field.path == "band.angleDeg") value = mGeometry.band.angleDeg * (mGeometry.mirrored ? -1.f : 1.f);
					else if (field.path == "band.offset") value = mGeometry.band.offset;
					else if (field.path == "band.width") value = mPipeline->EditorAssets().Settings().milkyWay.width;
					else if (field.path == "planet.at") value = {mGeometry.mirrored ? 1.f - mGeometry.planetAt.x : mGeometry.planetAt.x, mGeometry.planetAt.y};
					else if (field.path == "skyPaint.enabled" && !mEffective.contains("skyPaint")) value = false;
					else if (field.path.find("skyPaint.") == 0)
					{
						const auto palette = mPipeline->Palette();
						const sf::Glsl::Vec3* color = field.path == "skyPaint.shadow" ? &palette.shadow : field.path == "skyPaint.mid" ? &palette.mid : field.path == "skyPaint.lit" ? &palette.lit : field.path == "skyPaint.accent" ? &palette.accent : nullptr;
						if (color) value = {color->x, color->y, color->z};
					}
				}
				mRows.push_back({&field, field.path, value, mDocument->Authored(field.path) != nullptr, sky::ValidateSkyField(field, nlohmann::json(value))});
			}
		}
		const auto unknown = [&](const auto& self, const Json& value, const std::string& path) -> void
		{
			if (sky::FindSkyField(path) || path == "use") return;
			bool container = path.empty() || path == "objects";
			for (const auto& field : sky::DeepSkyFields())
				if (field.path.find(path + ".") == 0 || (path.find("objects.") == 0 && path.find('.', 8) == std::string::npos)) container = true;
			if (!container) { mUnknown.emplace_back(path, value.dump()); return; }
			for (auto it = value.begin(); it != value.end(); ++it)
			{
				const auto key = value.is_array() ? std::to_string(it - value.begin()) : it.key();
				self(self, it.value(), path.empty() ? key : path + '.' + key);
			}
		};
		unknown(unknown, mEffective, "");
		for (auto it = mDocument->document.Data().begin(); it != mDocument->document.Data().end(); ++it)
			if (it.key() != "stops" && it.key() != "skies" && it.key() != "sky" && it.key() != "id" && it.key() != "presets") mUnknown.emplace_back("Document." + it.key(), it->dump());
		mRowsRevision = mDocument->document.Revision(); mRowsSelection = selection;
	}

	void EditorSkyEditor::DrawFields(const std::string& block)
	{
		for (const auto& row : mRows)
		{
			const bool match = block.empty() ? row.path.find('.') == std::string::npos : row.path.find(block + '.') == 0 && row.path.find('.', block.size() + 1) == std::string::npos;
			if (!match) continue;
			const auto& field = *row.field;
			Json value = row.warning.empty() ? row.value : Json(field.defaultValue);
			const auto label = block.empty() ? row.path : row.path.substr(block.size() + 1);
			ImGui::PushID(row.path.c_str());
			ImGui::SetNextItemWidth(std::max(80.f, ImGui::GetContentRegionAvail().x - 100.f));
			bool changed = false;
			using Type = sky::SkyFieldType;
			const float min = field.min.value_or(-std::numeric_limits<float>::max());
			const float max = field.max.value_or(std::numeric_limits<float>::max());
			switch (field.type)
			{
			case Type::Number:
			{
				float v = value.get<float>(); changed = ImGui::DragFloat(label.c_str(), &v, .005f, min, max, "%.3f", ImGuiSliderFlags_AlwaysClamp); value = v; break;
			}
			case Type::Integer:
			{
				int v = value.get<int>(); changed = ImGui::DragInt(label.c_str(), &v, 1.f, static_cast<int>(field.min.value_or(0.f)), field.max ? static_cast<int>(*field.max) : INT_MAX, "%d", ImGuiSliderFlags_AlwaysClamp); value = v; break;
			}
			case Type::Bool:
			{
				bool v = value.get<bool>(); changed = ImGui::Checkbox(label.c_str(), &v); value = v; break;
			}
			case Type::Position:
			case Type::Color:
			{
				float v[3]{}; for (std::size_t i = 0; i < value.size(); ++i) v[i] = value[i].get<float>();
				changed = field.type == Type::Color ? ImGui::ColorEdit3(label.c_str(), v) : ImGui::DragFloat2(label.c_str(), v, .002f);
				for (std::size_t i = 0; i < value.size(); ++i) value[i] = v[i]; break;
			}
			case Type::Enum:
				if (ImGui::BeginCombo(label.c_str(), value.get_ref<const std::string&>().c_str()))
				{
					for (const auto& option : field.options) if (ImGui::Selectable(option.c_str(), option == value)) { value = option; changed = true; }
					ImGui::EndCombo();
				}
				break;
			case Type::Text: { std::string v = value.get<std::string>(); changed = InputStringProperty(label.c_str(), v); value = v; break; }
			}
			if (ImGui::IsItemActivated()) mGroup = mDocument->history.BeginMergeGroup();
			ImGui::SetItemTooltip("%s%s", row.path.c_str(), row.authored ? " (override)" : " (preset / generated default; viewing creates no override)");
			if (changed) mDocument->Set(row.path, value, field.type == Type::Enum || field.type == Type::Bool ? 0 : mGroup);
			if (ImGui::BeginPopupContextItem("Reset field"))
			{
				if (ImGui::MenuItem("Reset to preset / generated default", nullptr, false, row.authored)) mDocument->Reset(row.path);
				ImGui::EndPopup();
			}
			if (!row.warning.empty()) ImGui::TextWrapped("%s; stored: %s", row.warning.c_str(), row.value.dump().c_str());
			ImGui::PopID();
		}
	}

	void EditorSkyEditor::BlockTools(const std::string& path)
	{
		ImGui::PushID(path.c_str());
		if (ImGui::SmallButton("Copy block"))
		{
			const Json::json_pointer pointer{ContentPointer(path)};
			if (mEffective.contains(pointer)) { mClipboard = mEffective.at(pointer); mClipboardKind = BlockKind(path); }
		}
		ImGui::SameLine();
		ImGui::BeginDisabled(mClipboard.is_null() || mClipboardKind != BlockKind(path));
		if (ImGui::SmallButton("Paste block")) mDocument->Block(path, mClipboard);
		ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::SmallButton("Remove override")) mDocument->RemoveBlock(path);
		ImGui::PopID();
	}

	void EditorSkyEditor::Build(EditorContext& scene, CommandHistory& sceneHistory)
	{
		if (!mDocument || !mPipeline) return;
		if (mWasVisible && !visible && Dirty()) { visible = true; mConfirmClose = true; }
		mWasVisible = visible;
		if (visible)
		{
			mGeometry = mPipeline->Geometry(mOutput);
			if (mRefreshGeometryRows) { mRowsRevision = 0; mRefreshGeometryRows = false; }
			if (mFocusRequested) { ImGui::SetNextWindowFocus(); mFocusRequested = false; }
			bool open = true;
			if (ImGui::Begin(Dirty() ? "Sky *###SkyEditor" : "Sky###SkyEditor", &open))
			{
				const bool focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
				if (focused && !mWasFocused) mMode = true;
				mWasFocused = focused;
				if (ImGui::GetCurrentContext()->NavWindow)
				{
					const std::string focusedWindow = ImGui::GetCurrentContext()->NavWindow->RootWindow->Name;
					if (focusedWindow.find("###SkyEditor") == std::string::npos && focusedWindow != "Viewport" && !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel)) mMode = false;
				}
				ImGui::BeginDisabled(mSuspended || !mDocument->document.IsOpen());
				if (ImGui::Button("Save sky") && mDocument->Save()) mAppliedRevision = 0;
				ImGui::SameLine();
				if (ImGui::Button("Revert")) mConfirmClose = true;
				ImGui::SameLine();
				if (ImGui::Button("Undo")) mDocument->history.Undo();
				ImGui::SameLine(); if (ImGui::Button("Redo")) mDocument->history.Redo();
				if (ImGui::Checkbox("Sky handles", &mMode)) mDragHandle.clear();
				ImGui::SameLine(); ImGui::Checkbox("Snap", &mSnap);
				ImGui::TextDisabled("Drag handles in Viewport. Hold B: saved sky. Right-click a field: reset.");
				const auto& stops = mPipeline->Skies();
				ImGui::SetNextItemWidth(std::max(100.f, ImGui::GetContentRegionAvail().x - 100.f));
				if (!stops.empty() && ImGui::BeginCombo("Preview sky", stops[mSkyIndex].id.c_str()))
				{
					for (const auto& stop : stops) if (ImGui::Selectable((stop.id + " - " + stop.name).c_str(), stop.id == stops[mSkyIndex].id)) Open(stop.id);
					ImGui::EndCombo();
				}
				if (ImGui::Button("Use in scene")) mPipeline->UseInScene(sceneHistory, mSkyIndex);
				if (mCompare) ImGui::TextColored({1.f, .8f, .3f, 1.f}, "A: saved sky (release B to return to edits)");
				if (mGeometry.mirrored) ImGui::TextWrapped("Mirrored sky: planet X and band angle mirror; objects follow the mirror switch. Drags edit authored coordinates.");
				if (mSuspended) ImGui::TextWrapped("Sky authoring is available in scene Edit mode.");
				if (mRebuilding) ImGui::TextColored({1.f, .8f, .3f, 1.f}, "Rebuilding orbit panorama after gesture settles...");
				ImGui::TextDisabled("Parse %.2f ms | apply %.2f ms | render %.2f ms | thumbnail %.2f ms", mDocument->parseMs, mApplyMs, mRenderMs, mThumbnailMs);
				CacheRows();
				if (mDocument->document.IsOpen())
				{
					Thumbnails(scene);
					if (ImGui::CollapsingHeader("Presets and duplicate"))
					{
						if (ImGui::BeginCombo("Preset", mDocument->preset ? mDocument->selected.c_str() : "Choose"))
						{
							if (mDocument->document.Data().contains("presets"))
								for (auto it = mDocument->document.Data()["presets"].begin(); it != mDocument->document.Data()["presets"].end(); ++it)
								{
									if (ImGui::Selectable(it.key().c_str())) { mDocument->Select(it.key(), true); mRowsRevision = mAppliedRevision = mObservedRevision = 0; }
								}
							ImGui::EndCombo();
						}
						if (mDocument->preset)
						{
							ImGui::TextWrapped("Editing preset %s. Referencing skies update together.", mDocument->selected.c_str());
							if (ImGui::Button("Apply preset to preview sky"))
							{
								const auto name = mDocument->selected; mDocument->Select(stops[mSkyIndex].id); mDocument->Set("use", name); mRowsRevision = mAppliedRevision = mObservedRevision = 0;
							}
						}
						else if (mDocument->Authored() && mDocument->Authored()->is_object() && mDocument->Authored()->contains("use"))
							ImGui::TextWrapped("Uses preset %s. Editing inherited objects detaches this stop to avoid appending duplicates.", mDocument->Authored()->at("use").get_ref<const std::string&>().c_str());
						ImGui::InputText("Preset name", mPresetName, sizeof(mPresetName));
						if (ImGui::Button("Store as preset")) mDocument->StorePreset(mPresetName);
						if (!stops.empty() && ImGui::BeginCombo("Duplicate target", stops[mDuplicateTarget].id.c_str()))
						{
							for (std::size_t i = 0; i < stops.size(); ++i) if (ImGui::Selectable(stops[i].id.c_str(), i == mDuplicateTarget)) mDuplicateTarget = i;
							ImGui::EndCombo();
						}
						if (ImGui::Button("Duplicate sky to target")) mDocument->DuplicateTo(stops[mDuplicateTarget].id);
					}
					DrawFields("");
					for (const char* block : {"band", "skyPaint", "planet", "orbit", "hostStar", "grade", "asteroids"})
					{
						if (!ImGui::CollapsingHeader(block)) continue;
						const bool exists = mEffective.contains(block);
						if (!exists) ImGui::TextDisabled("No override. Editing creates this block.");
						BlockTools(block); DrawFields(block);
						if (std::string(block) == "orbit" && ImGui::TreeNode("Ring")) { BlockTools("orbit.ring"); DrawFields("orbit.ring"); ImGui::TreePop(); }
					}
					if (ImGui::CollapsingHeader("Objects"))
					{
						if (mEffective.contains("orbit")) ImGui::TextWrapped("Orbit stops do not render objects, skyPaint, nebula or asteroids.");
						if (ImGui::Button("Add object"))
						{
							auto objects = mEffective.value("objects", Json::array()); objects.push_back(Json{{"kind", "emission"}}); mDocument->Block("objects", objects, "Add sky object");
						}
						if (mEffective.contains("objects")) for (std::size_t i = 0; i < mEffective["objects"].size(); ++i)
						{
							const auto path = "objects." + std::to_string(i);
							if (ImGui::TreeNode(path.c_str()))
							{
								BlockTools(path); DrawFields(path);
								if (ImGui::Button("Duplicate object")) { auto objects = mEffective["objects"]; objects.push_back(objects[i]); mDocument->Block("objects", objects, "Duplicate sky object"); }
								ImGui::TreePop();
							}
						}
					}
					if (!mUnknown.empty() && ImGui::CollapsingHeader("Other (read-only)"))
						for (const auto& [path, text] : mUnknown) ImGui::TextWrapped("%s: %s", path.c_str(), text.c_str());
				}
				ImGui::EndDisabled();
				if (!mDocument->error.empty()) ImGui::TextWrapped("%s", mDocument->error.c_str());
			}
			ImGui::End();
			if (!open) { if (Dirty()) mConfirmClose = true; else visible = mWasVisible = false; }
		}
		mCompare = Active() && !ImGui::GetIO().WantTextInput && ImGui::IsKeyDown(ImGuiKey_B);
		if (Active() && ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false) && mDocument->Save()) mAppliedRevision = 0;
		if (mConfirmClose) { ImGui::OpenPopup("Unsaved sky"); mConfirmClose = false; }
		if (ImGui::BeginPopupModal("Unsaved sky", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Save changes to %s?", mPipeline->DocumentFile().generic_string().c_str());
			bool finish = false;
			if (ImGui::Button("Save")) finish = mDocument->Save();
			ImGui::SameLine();
			if (ImGui::Button("Discard")) finish = mDocument->Reopen();
			ImGui::SameLine();
			if (ImGui::Button("Cancel")) { mAfterClose = {}; ImGui::CloseCurrentPopup(); }
			if (finish)
			{
				visible = mWasVisible = false; EndPreview(scene); ImGui::CloseCurrentPopup();
				if (mAfterClose) { auto callback = std::move(mAfterClose); mAfterClose = {}; callback(); }
			}
			if (!mDocument->error.empty()) ImGui::TextWrapped("%s", mDocument->error.c_str());
			ImGui::EndPopup();
		}
	}

	void EditorSkyEditor::RequestExit(std::function<void()> continuation)
	{
		mAfterClose = std::move(continuation); mConfirmClose = true;
	}

	bool EditorSkyEditor::RenderSky(sf::RenderTexture& target, std::size_t index, const ly::World& world, sf::Vector2u size)
	{
		const auto start = std::chrono::steady_clock::now();
		const bool result = mPipeline->RenderSky(target, index, world, size);
		mRenderMs = Milliseconds(start);
		return result;
	}

	void EditorSkyEditor::Thumbnails(EditorContext& scene)
	{
		if (!scene.world || mSuspended || mRebuilding || mCompare) return;
		if (ImGui::BeginChild("Sky thumbnails", {0.f, 106.f}, ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar))
		{
			bool rendered = false;
			for (const auto& stop : mPipeline->Skies())
			{
				ImGui::PushID(stop.id.c_str());
				ImGui::BeginGroup();
				const auto position = ImGui::GetCursorScreenPos();
				auto& thumb = mThumbnails[stop.id];
				if (ImGui::IsRectVisible({144.f, 81.f}) && (!thumb.target || thumb.revision != mAppliedRevision) && !rendered)
				{
					const auto start = std::chrono::steady_clock::now();
					if (!thumb.target) thumb.target = std::make_unique<sf::RenderTexture>();
					if (RenderSky(*thumb.target, static_cast<std::size_t>(&stop - mPipeline->Skies().data()), *scene.world, {160, 90})) thumb.revision = mAppliedRevision;
					mThumbnailMs = Milliseconds(start); rendered = true;
				}
				if (thumb.target && thumb.revision == mAppliedRevision) ImGui::Image(*thumb.target, {144.f, 81.f});
				else ImGui::Dummy({144.f, 81.f});
				if (ImGui::IsItemClicked()) Open(stop.id);
				ImGui::SetCursorScreenPos(position);
				if (ImGui::InvisibleButton("Pick sky", {144.f, 81.f})) Open(stop.id);
				ImGui::TextUnformatted(stop.id.c_str()); ImGui::EndGroup(); ImGui::PopID(); ImGui::SameLine();
			}
		}
		ImGui::EndChild();
	}
}
