#include "editor/EditorApplication.h"
#include "editor/JsonCommands.h"
#include <imgui.h>
#include <imgui-SFML.h>
#include <algorithm>
#include <cctype>

namespace vega
{
	namespace
	{
		std::string Lower(std::string text)
		{
			for (char& c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
			return text;
		}
		const char* FileLabel(const std::filesystem::directory_entry& entry)
		{
			std::error_code error;
			if (entry.is_directory(error)) return "[Folder]";
			const auto extension = Lower(entry.path().extension().string());
			if (extension == ".json") return "{JSON}";
			if (extension == ".png" || extension == ".jpg" || extension == ".jpeg") return "[Texture]";
			if (extension == ".ogg" || extension == ".wav" || extension == ".flac") return "[Audio]";
			if (extension == ".ttf" || extension == ".otf") return "[Font]";
			return "[File]";
		}
	}

	void EditorApplication::RefreshContent()
	{
		mContentEntries.clear();
		mContentError.clear();
		std::filesystem::path path;
		if (!mDocument.Resolve(mContentDirectory, path)) { mContentError = "Directory escapes the asset root"; return; }
		std::error_code error;
		for (std::filesystem::directory_iterator it{ path, error }, end; !error && it != end; it.increment(error))
		{
			std::filesystem::path safe;
			if (mDocument.Resolve(it->path().lexically_relative(mDocument.Root()), safe)) mContentEntries.push_back(*it);
		}
		if (error) mContentError = error.message();
		std::sort(mContentEntries.begin(), mContentEntries.end(), [](const auto& a, const auto& b)
		{
			std::error_code ae, be;
			const bool ad = a.is_directory(ae), bd = b.is_directory(be);
			return ad != bd ? ad : Lower(a.path().filename().string()) < Lower(b.path().filename().string());
		});
	}

	void EditorApplication::DrawDirectory(const std::filesystem::path& relative, unsigned int depth)
	{
		std::filesystem::path path;
		if (!mDocument.Resolve(relative, path)) return;
		const auto key = relative.generic_string();
		ImGui::PushID(key.c_str());
		const auto name = relative.empty() ? "Assets" : relative.filename().string();
		const bool open = ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_OpenOnArrow |
			(relative.empty() ? ImGuiTreeNodeFlags_DefaultOpen : 0) |
			(relative == mContentDirectory ? ImGuiTreeNodeFlags_Selected : 0));
		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) { mContentDirectory = relative; RefreshContent(); }
		if (open)
		{
			// Do not traverse linked directories: cycles and external asset trees have no place here.
			std::error_code error;
			std::vector<std::filesystem::path> children;
			for (std::filesystem::directory_iterator it{ path, error }, end; !error && it != end; it.increment(error))
			{
				std::error_code status;
				if (depth < 64 && !it->is_symlink(status) && it->is_directory(status))
					children.push_back(it->path().lexically_relative(mDocument.Root()));
			}
			std::sort(children.begin(), children.end());
			for (const auto& child : children) DrawDirectory(child, depth + 1);
			ImGui::TreePop();
		}
		ImGui::PopID();
	}

	void EditorApplication::RequestDocument(const std::filesystem::path& relative)
	{
		mPendingDocument = relative;
		if (mDocument.Dirty()) mConfirmDocument = true;
		else FinishDocumentRequest();
	}

	void EditorApplication::FinishDocumentRequest()
	{
		if (mPendingQuit) QuitApplication();
		else if (mPendingClose) { mDocument.Close(); mShowData = false; }
		else if (mDocument.Open(mPendingDocument)) mShowData = true;
		mPendingDocument.clear();
		mPendingClose = mPendingQuit = false;
		mJsonMergeGroup = 0;
	}

	void EditorApplication::BuildContentPanel()
	{
		mRetiredThumbnails.clear();
		if (ImGui::Begin("Content", &mShowContent))
		{
			ImGui::TextWrapped("Asset root: %s", mDocument.Root().string().c_str());
			if (ImGui::Button("Refresh")) { mThumbnails.clear(); RefreshContent(); }
			ImGui::SameLine();
			ImGui::InputText("Search", mContentFilter, sizeof(mContentFilter));
			if (ImGui::BeginChild("Directories", { ImGui::GetContentRegionAvail().x * 0.28f, 0.f }, ImGuiChildFlags_Borders))
				DrawDirectory({});
			ImGui::EndChild();
			ImGui::SameLine();
			if (ImGui::BeginChild("Entries", {}, ImGuiChildFlags_Borders))
			{
				ImGui::TextUnformatted(mContentDirectory.empty() ? "/" : mContentDirectory.generic_string().c_str());
				if (!mContentDirectory.empty() && ImGui::Selectable("[Folder] ..", false, ImGuiSelectableFlags_AllowDoubleClick) && ImGui::IsMouseDoubleClicked(0))
				{
					mContentDirectory = mContentDirectory.parent_path();
					RefreshContent();
				}
				if (!mContentError.empty()) ImGui::TextWrapped("%s", mContentError.c_str());
				std::filesystem::path enter;
				bool enterFolder = false;
				for (const auto& entry : mContentEntries)
				{
					if (Lower(entry.path().filename().string()).find(Lower(mContentFilter)) == std::string::npos) continue;
					const auto relative = entry.path().lexically_relative(mDocument.Root());
					std::filesystem::path safe;
					if (!mDocument.Resolve(relative, safe)) continue;
					ImGui::PushID(relative.generic_string().c_str());
					const auto extension = Lower(entry.path().extension().string());
					// Only visible rows acquire a texture. A timestamp change invalidates even failed loads.
					if ((extension == ".png" || extension == ".jpg" || extension == ".jpeg") && ImGui::IsRectVisible({ 32.f, 32.f }))
					{
						std::error_code error;
						const auto modified = std::filesystem::last_write_time(safe, error);
						if (!error)
						{
							auto found = mThumbnails.find(safe);
							if (found == mThumbnails.end() || found->second.modified != modified)
							{
								if (mThumbnails.size() >= 64)
								{
									mRetiredThumbnails.push_back(std::move(mThumbnails.begin()->second.texture));
									mThumbnails.erase(mThumbnails.begin());
									found = mThumbnails.find(safe);
								}
								if (found != mThumbnails.end()) mRetiredThumbnails.push_back(std::move(found->second.texture));
								Thumbnail thumbnail{ modified, nullptr };
								// Bound GPU memory as well as entry count by resizing the decoded image.
								sf::Image image;
								if (image.loadFromFile(safe))
								{
									const auto size = image.getSize();
									const float scale = std::min(1.f, 64.f / static_cast<float>(std::max(size.x, size.y)));
									sf::Image small{ sf::Vector2u{ std::max(1u, static_cast<unsigned int>(size.x * scale)), std::max(1u, static_cast<unsigned int>(size.y * scale)) } };
									for (unsigned int y = 0; y < small.getSize().y; ++y)
										for (unsigned int x = 0; x < small.getSize().x; ++x)
											small.setPixel({ x, y }, image.getPixel({ x * size.x / small.getSize().x, y * size.y / small.getSize().y }));
									thumbnail.texture = std::make_unique<sf::Texture>();
									if (!thumbnail.texture->loadFromImage(small)) thumbnail.texture.reset();
								}
								mThumbnails[safe] = std::move(thumbnail);
								found = mThumbnails.find(safe);
							}
							if (found->second.texture)
							{
								const auto size = found->second.texture->getSize();
								const float scale = 32.f / static_cast<float>(std::max(size.x, size.y));
								ImGui::Image(*found->second.texture, sf::Vector2f{ size.x * scale, size.y * scale });
								ImGui::SameLine();
							}
						}
					}
					const auto label = std::string{ FileLabel(entry) } + " " + entry.path().filename().string();
					if (ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick, { 0.f, 32.f }) && ImGui::IsMouseDoubleClicked(0))
					{
						std::error_code error;
						if (entry.is_directory(error)) { enter = relative; enterFolder = true; }
						else if (extension == ".json") RequestDocument(relative);
					}
					ImGui::PopID();
				}
				if (enterFolder) { mContentDirectory = enter; RefreshContent(); }
			}
			ImGui::EndChild();
		}
		ImGui::End();
	}

	void EditorApplication::DrawJsonNode(const Json& value, const std::string& pointer, const std::string& label)
	{
		ImGui::PushID(pointer.c_str());
		if (value.is_structured())
		{
			const auto title = label + " (" + std::to_string(value.size()) + (value.is_object() ? " keys)" : " items)");
			if (ImGui::TreeNode(title.c_str()))
			{
				for (auto it = value.begin(); it != value.end(); ++it)
				{
					const auto key = value.is_object() ? it.key() : std::to_string(it - value.begin());
					DrawJsonNode(it.value(), pointer + "/" + JsonDocument::EscapePointer(key), value.is_object() ? key : "[" + key + "]");
				}
				ImGui::TreePop();
			}
		}
		else
		{
			Json after = value;
			bool changed = false;
			if (value.is_boolean())
			{
				bool boolean = value.get<bool>();
				changed = ImGui::Checkbox(label.c_str(), &boolean);
				after = boolean;
			}
			else if (value.is_number_unsigned())
			{
				std::uint64_t number = value.get<std::uint64_t>();
				changed = ImGui::InputScalar(label.c_str(), ImGuiDataType_U64, &number);
				after = number;
			}
			else if (value.is_number_integer())
			{
				std::int64_t number = value.get<std::int64_t>();
				changed = ImGui::InputScalar(label.c_str(), ImGuiDataType_S64, &number);
				after = number;
			}
			else if (value.is_number_float())
			{
				double number = value.get<double>();
				changed = ImGui::InputDouble(label.c_str(), &number, 0., 0., "%.17g");
				after = number;
			}
			else if (value.is_string())
			{
				auto text = value.get<std::string>();
				changed = ImGui::InputText(label.c_str(), text.data(), text.capacity() + 1, ImGuiInputTextFlags_CallbackResize,
					[](ImGuiInputTextCallbackData* data)
					{
						auto& buffer = *static_cast<std::string*>(data->UserData);
						buffer.resize(static_cast<std::size_t>(data->BufTextLen));
						data->Buf = buffer.data();
						return 0;
					}, &text);
				after = text;
			}
			else ImGui::Text("%s: null", label.c_str());
			if (!value.is_null())
			{
				if (ImGui::IsItemActivated()) mJsonMergeGroup = mHistory.BeginMergeGroup();
				if (changed && after != value)
					mHistory.Execute(std::make_unique<SetJsonValueCommand>(mDocument.ID(), pointer, value, after, mJsonMergeGroup));
				if (ImGui::IsItemDeactivated()) mJsonMergeGroup = 0;
			}
		}
		ImGui::PopID();
	}

	void EditorApplication::BuildDataPanel()
	{
		if (mDocument.IsOpen() && mPresentedDocumentID != mDocument.ID())
		{
			mShowData = true;
			mPresentedDocumentID = mDocument.ID();
			ImGui::SetNextWindowFocus();
		}
		if (mShowData)
		{
			bool show = true;
			const std::string title = mDocument.Dirty() ? "Data Editor *###DataEditor" : "Data Editor###DataEditor";
			if (ImGui::Begin(title.c_str(), &show))
			{
				ImGui::TextWrapped("%s", mDocument.IsOpen() ? mDocument.Path().string().c_str() : "Open a JSON file from Content");
				ImGui::TextUnformatted("Changes apply to the game after restart");
				if (mDocument.ReadOnly()) ImGui::TextWrapped("%s", mDocument.ReadOnlyReason().c_str());
				if (!mDocument.Error().empty()) ImGui::TextWrapped("%s", mDocument.Error().c_str());
				ImGui::BeginDisabled(!mDocument.IsOpen() || mDocument.ReadOnly());
				if (ImGui::Button("Save") || (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
					ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false))) mDocument.Save();
				ImGui::EndDisabled();
				ImGui::SameLine();
				ImGui::BeginDisabled(!mDocument.IsOpen());
				if (ImGui::Button("Revert")) { mDocument.Revert(); mJsonMergeGroup = 0; }
				ImGui::SameLine();
				if (ImGui::Button("Close")) show = false;
				ImGui::EndDisabled();
				ImGui::Separator();
				ImGui::BeginDisabled(mDocument.ReadOnly());
				if (mDocument.IsOpen()) DrawJsonNode(mDocument.Data(), "", "Document");
				ImGui::EndDisabled();
			}
			ImGui::End();
			if (!show)
			{
				mPendingClose = true;
				if (mDocument.Dirty()) mConfirmDocument = true;
				else FinishDocumentRequest();
			}
		}
		if (mConfirmDocument)
		{
			ImGui::OpenPopup("Unsaved JSON changes");
			mConfirmDocument = false;
		}
		if (ImGui::BeginPopupModal("Unsaved JSON changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::TextUnformatted("Save changes before continuing?");
			if (!mDocument.Error().empty()) ImGui::TextWrapped("%s", mDocument.Error().c_str());
			if (ImGui::Button("Save"))
			{
				if (mDocument.Save()) { FinishDocumentRequest(); ImGui::CloseCurrentPopup(); }
			}
			ImGui::SameLine();
			if (ImGui::Button("Discard")) { FinishDocumentRequest(); ImGui::CloseCurrentPopup(); }
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				mPendingClose = mPendingQuit = false;
				mPendingDocument.clear();
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}
}
