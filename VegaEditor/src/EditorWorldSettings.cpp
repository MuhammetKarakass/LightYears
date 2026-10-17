#include "editor/EditorApplication.h"
#include "editor/PropertyWidgets.h"
#include <imgui_internal.h>
#include <cstring>
#include <algorithm>

namespace vega
{
	void EditorApplication::InstallWorldSettingsLayout()
	{
		ImGuiSettingsHandler handler;
		handler.TypeName = "VegaWorldSettings";
		handler.TypeHash = ImHashStr(handler.TypeName);
		handler.UserData = this;
		handler.ReadOpenFn = [](ImGuiContext*, ImGuiSettingsHandler* h, const char*) -> void* { return h->UserData; };
		handler.ReadLineFn = [](ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line)
		{
			auto& self = *static_cast<EditorApplication*>(entry);
			if (std::strncmp(line, "Visible=", 8) == 0) self.mShowWorldSettings = line[8] == '1';
		};
		handler.WriteAllFn = [](ImGuiContext*, ImGuiSettingsHandler* h, ImGuiTextBuffer* out)
		{
			const auto& self = *static_cast<EditorApplication*>(h->UserData);
			out->appendf("[VegaWorldSettings][Panel]\nVisible=%d\n\n", self.mShowWorldSettings);
		};
		ImGui::AddSettingsHandler(&handler);
	}

	void EditorApplication::BuildWorldSettingsPanel()
	{
		if (ImGui::Begin("World Settings", &mShowWorldSettings))
		{
			const auto& schema = GetEngineContext().SceneSettings();
			// JSON previews, validation and catalog enumeration only run after edits
			// or document/schema replacement, never on every UI frame.
			if (mSettingsPanelRevision != mContext.sceneRevision || mSettingsSchemaRevision != schema.Revision())
			{
				mSettingsRows.clear();
				mOtherSettings.clear();
				for (const auto& field : schema.Fields())
				{
					SettingRow row;
					row.field = field;
					row.value = schema.Value(mSceneSettings, field.key);
					row.authored = mSceneSettings.contains(field.key);
					row.warning = field.Validate(row.value);
					row.options = field.options ? field.options() : std::vector<ly::SceneSettingOption>{};
					row.preview = row.value.dump();
					for (const auto& option : row.options) if (option.value == row.value) row.preview = option.label;
					mSettingsRows[field.category.empty() ? "General" : field.category].push_back(std::move(row));
				}
				for (const auto& entry : mSceneSettings.items())
					if (!schema.Find(entry.key())) mOtherSettings.emplace_back(entry.key(), entry.value().dump());
				mSettingsPanelRevision = mContext.sceneRevision;
				mSettingsSchemaRevision = schema.Revision();
			}
			if (mContext.playMode) ImGui::TextWrapped("Play inspection only. Stop to edit scene settings.");
			ImGui::BeginDisabled(mContext.playMode || mContext.transitioning || mPrefabWorld != nullptr);
			for (const auto& category : mSettingsRows)
			{
				if (!ImGui::CollapsingHeader(category.first.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) continue;
				for (const auto& row : category.second)
				{
					const auto& field = row.field;
					ImGui::PushID(field.key.c_str());
					auto value = row.warning.empty() ? row.value : field.defaultValue;
					const auto* label = field.label.empty() ? field.key.c_str() : field.label.c_str();
					bool changed = false;
					using Type = ly::SceneSettingType;
					const float min = static_cast<float>(field.min.value_or(0));
					const float max = static_cast<float>(field.max.value_or(0));
					switch (field.type)
					{
					case Type::Bool:
					{
						auto v = value.get<bool>();
						changed = ImGui::Checkbox(label, &v); value = v; break;
					}
					case Type::Int:
					{
						auto v = value.get<int>();
						changed = ImGui::DragInt(label, &v, 1.f, static_cast<int>(field.min.value_or(0)), static_cast<int>(field.max.value_or(0))); value = v; break;
					}
					case Type::Float:
					{
						auto v = value.get<float>();
						changed = ImGui::DragFloat(label, &v, .1f, min, max); value = v; break;
					}
					case Type::String:
					{
						auto v = value.get<std::string>();
						changed = InputStringProperty(label, v); value = v; break;
					}
					case Type::Enum:
					case Type::CatalogId:
						if (ImGui::BeginCombo(label, row.preview.c_str()))
						{
							for (const auto& option : row.options)
								if (ImGui::Selectable(option.label.c_str(), option.value == row.value)) { value = option.value; changed = true; }
							ImGui::EndCombo();
						}
						break;
					case Type::Vec2:
					{
						float v[]{ value[0].get<float>(), value[1].get<float>() };
						changed = ImGui::DragFloat2(label, v, .1f, min, max); value = { v[0], v[1] }; break;
					}
					case Type::Rect:
					{
						bool enabled = !value.is_null();
						changed = ImGui::Checkbox(label, &enabled);
						if (changed) mSettingMergeGroup = mHistory.BeginMergeGroup();
						if (changed) value = enabled ? nlohmann::json{ 0.f, 0.f, 1000.f, 1000.f } : nlohmann::json{};
						if (enabled)
						{
							float v[]{ value[0].get<float>(), value[1].get<float>(), value[2].get<float>(), value[3].get<float>() };
							changed = ImGui::DragFloat4("X / Y / Width / Height", v, 1.f, min, max) || changed;
							value = { v[0], v[1], v[2], v[3] };
						}
						break;
					}
					case Type::Color:
					{
						float v[4];
						for (int i = 0; i < 4; ++i) v[i] = value[i].get<float>() / 255.f;
						changed = ImGui::ColorEdit4(label, v);
						for (int i = 0; i < 4; ++i) value[i] = static_cast<int>(std::clamp(v[i], 0.f, 1.f) * 255.f + .5f);
						break;
					}
					}
					if (ImGui::IsItemActivated()) mSettingMergeGroup = mHistory.BeginMergeGroup();
					if (!field.tooltip.empty()) ImGui::SetItemTooltip("%s", field.tooltip.c_str());
					if (changed && (value != row.value || !row.warning.empty())) mHistory.Execute(std::make_unique<SetSceneSettingCommand>(field.key, value,
						field.type == Type::Enum || field.type == Type::CatalogId || field.type == Type::Bool ? 0 : mSettingMergeGroup));
					ImGui::SameLine();
					ImGui::BeginDisabled(!row.authored);
					if (ImGui::SmallButton("Reset")) mHistory.Execute(std::make_unique<SetSceneSettingCommand>(field.key, std::nullopt));
					ImGui::EndDisabled();
					if (!row.authored) ImGui::TextDisabled("Default (no override)");
					if (!row.warning.empty()) ImGui::TextWrapped("Warning: %s. Stored: %s", row.warning.c_str(), row.preview.c_str());
					if (mContext.settingErrorKey == field.key && !mContext.settingError.empty()) ImGui::TextWrapped("Warning: %s", mContext.settingError.c_str());
					ImGui::PopID();
				}
			}
			ImGui::EndDisabled();
			if (!mOtherSettings.empty() && ImGui::CollapsingHeader("Other", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::TextDisabled("Unknown settings are preserved and read-only.");
				for (const auto& entry : mOtherSettings) ImGui::TextWrapped("%s: %s", entry.first.c_str(), entry.second.c_str());
			}
		}
		ImGui::End();
	}
}
