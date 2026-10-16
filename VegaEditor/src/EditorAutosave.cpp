#include "editor/EditorApplication.h"
#include <framework/scene/PrefabLibrary.h>
#include <imgui.h>
#include <algorithm>

namespace vega
{
	void EditorApplication::PollContentReload()
	{
		const auto now = std::chrono::steady_clock::now();
		if (now < mNextContentPoll || mContext.transitioning) return;
		mNextContentPoll = now + std::chrono::milliseconds{ 500 };
		const auto paths = mContentWatch.Poll();
		if (paths.empty()) return;
		ly::PrefabLibrary::Get().Reload();
		try { if (auto* reload = QueryCapability<IContentReload>(mModule.get())) reload->ReloadContent(paths); }
		catch (const std::exception& error)
		{
			ly::debug::Log::Write(ly::debug::LogLevel::Error, ly::debug::LogChannel::Editor, LY_SOURCE_LOCATION,
				std::string{ "Content reload failed: " } + error.what());
		}
		for (const auto& path : paths)
			ly::debug::Log::Write(ly::debug::LogLevel::Info, ly::debug::LogChannel::Editor, LY_SOURCE_LOCATION,
				"Content change dispatched: " + path.lexically_relative(mDocument.Root()).generic_string());
	}

	bool EditorApplication::AutosaveNow()
	{
		if (!mAutosaves.Enabled()) { mAutosaveError = "Autosave requires a writable disposable asset root in script mode"; return false; }
		if (mContext.transitioning) { mAutosaveError = "Wait for the scene transition before autosaving"; return false; }
		bool success = true;
		mAutosaveError.clear();
		const auto save = [&](const std::filesystem::path& path, const std::string& kind, const std::string& text)
		{
			if (!mAutosaves.Write(path, kind, text))
			{
				success = false;
				mAutosaveError += path.generic_string() + ": " + mAutosaves.Error() + "\n";
			}
		};
		try
		{
			// CaptureScene returns the authored snapshot during Play, never runtime actors.
			if (SceneDirty()) save(mScenePath.empty() ? std::filesystem::path{ "scenes/untitled.scene.json" } : mScenePath,
				"scene", CaptureScene().dump(1, '\t') + "\n");
			if (PrefabDirty())
			{
				const auto prefab = CapturePrefab();
				if (prefab.is_null()) throw std::runtime_error("Cannot capture prefab (its parent may be missing)");
				save(mPrefabPath, "prefab", prefab.dump(1, '\t') + "\n");
			}
			if (mDocument.Dirty() && !mDocument.ReadOnly()) save(mDocument.Path(), "json", mDocument.Serialize());
			for (const auto& [path, text] : mContentAuthoring.DirtyDocuments()) save(path, "content", text);
		}
		catch (const std::exception& error) { success = false; mAutosaveError += error.what(); }
		ly::debug::Log::Write(success ? ly::debug::LogLevel::Info : ly::debug::LogLevel::Error,
			ly::debug::LogChannel::Editor, LY_SOURCE_LOCATION, success ? "Autosave completed" : mAutosaveError);
		return success;
	}

	void EditorApplication::PollAutosave()
	{
		const auto seconds = mProject.settings.AutosaveSeconds();
		if (!mAutosaves.Enabled() || !seconds || mContext.transitioning) return;
		const auto now = std::chrono::steady_clock::now();
		if (now < mNextAutosave) return;
		mNextAutosave = now + std::chrono::seconds{ seconds };
		AutosaveNow();
	}

	bool EditorApplication::RecoverAutosave(const AutosaveEntry& requested)
	{
		if (mContext.playMode || mContext.transitioning) { mAutosaveError = "Stop Play and wait for the scene before recovery"; return false; }
		// Re-read and validate the envelope at the moment of recovery. A stale dialog
		// must not trust paths or text that an external process has since replaced.
		const auto entries = mAutosaves.Entries();
		const auto found = std::find_if(entries.begin(), entries.end(), [&](const auto& item) { return item.snapshot == requested.snapshot; });
		if (found == entries.end()) { mAutosaveError = "Autosave is unavailable"; return false; }
		const auto& entry = *found;
		mAutosaveError.clear();
		try
		{
			bool success = false;
			if (entry.kind == "scene")
			{
				if (SceneDirty() || PrefabDirty()) throw std::runtime_error("Save or discard the current scene/prefab before recovery");
				const nlohmann::json scene = nlohmann::json::parse(entry.text);
				success = OpenScene(entry.source, true, &scene);
				if (!success) mAutosaveError = mContext.sceneError;
			}
			else if (entry.kind == "prefab")
			{
				if (PrefabDirty()) throw std::runtime_error("Save or discard the current prefab before recovery");
				// Resolve through the normal prefab loader without replacing the original.
				// Nested references remain relative to the asset root, as in authored files.
				const auto staging = mAutosaves.Session() / "recovered.prefab.json";
				std::filesystem::path safe;
				if (!mDocument.Resolve(staging.lexically_relative(mDocument.Root()), safe) || safe != staging)
					throw std::runtime_error("Recovery staging directory changed");
				if (!AtomicWriteText(safe, entry.text, mAutosaveError)) return false;
				ly::PrefabLibrary::Get().Reload();
				success = OpenPrefab(staging.lexically_relative(mDocument.Root()));
				if (success)
				{
					mPrefabPath = entry.source;
					mPrefabSaved = nullptr;
					mPrefabDirty = true;
				}
				else mAutosaveError = mContext.sceneError;
			}
			else if (entry.kind == "content")
			{
				success = mContentAuthoring.Recover(mDocument.Root(), entry.source, entry.text);
				if (!success && !mContentAuthoring.Dirty() && !mDocument.Dirty())
				{
					// Removed catalogs can still be reviewed in the generic data editor.
					success = mDocument.Recover(entry.source, entry.text);
					if (success) mShowData = true;
				}
				if (!success) mAutosaveError = mContentAuthoring.Error();
			}
			else if (entry.kind == "json")
			{
				if (mDocument.Dirty()) throw std::runtime_error("Save or discard the current data document before recovery");
				success = mDocument.Recover(entry.source, entry.text);
				if (success) mShowData = true;
				else mAutosaveError = mDocument.Error();
			}
			else throw std::runtime_error("Unknown autosave document kind");
			if (success)
			{
				// Keep the recovered buffer crash-recoverable immediately, including a
				// second crash before the next periodic deadline.
				if (!mAutosaves.Write(entry.source, entry.kind, entry.text)) mAutosaveError = mAutosaves.Error();
				else if (entry.snapshot.parent_path() != mAutosaves.Session() && !mAutosaves.Acknowledge(entry)) mAutosaveError = mAutosaves.Error();
				ly::debug::Log::Write(ly::debug::LogLevel::Info, ly::debug::LogChannel::Editor, LY_SOURCE_LOCATION,
					"Recovered into an unsaved editor buffer: " + entry.source.generic_string());
			}
			return success;
		}
		catch (const std::exception& error) { mAutosaveError = error.what(); return false; }
	}

	bool EditorApplication::ExecuteAutosaveCommand(const EditorScriptCommand& command, std::string& detail)
	{
		bool success = false;
		if (command.operation == "autosave-now") success = AutosaveNow();
		else if (command.operation == "simulate-unclean-exit")
		{
			success = mAutosaves.Enabled();
			if (success) mAutosaves.SimulateUncleanExit();
		}
		else
		{
			const auto requested = std::filesystem::u8path(command.text);
			std::filesystem::path absolute;
			const auto relative = requested.is_absolute() ? requested.lexically_relative(mDocument.Root()) : requested;
			if (!mDocument.Resolve(relative, absolute)) { detail = "Path escapes the asset root"; return false; }
			const auto entries = command.operation == "expect-recovery" ? mRecoveryEntries : mAutosaves.Entries();
			const auto found = std::find_if(entries.begin(), entries.end(), [&](const auto& entry)
			{
				return mDocument.Root() / entry.source == absolute;
			});
			if (command.operation == "recover-autosave")
				success = found != entries.end() && RecoverAutosave(*found);
			else success = (found != entries.end()) == command.overwrite;
		}
		detail = success ? "Autosave command completed" : mAutosaveError.empty() ? "Autosave expectation failed" : mAutosaveError;
		return success;
	}

	void EditorApplication::BuildRecoveryDialog()
	{
		if (mRecoveryRequested)
		{
			mRecoveryRequested = false;
			ImGui::OpenPopup("Recover Autosaves");
		}
		ImGui::SetNextWindowSize({ 760.f, 480.f }, ImGuiCond_Appearing);
		if (!ImGui::BeginPopupModal("Recover Autosaves", nullptr, ImGuiWindowFlags_NoSavedSettings)) return;
		ImGui::TextWrapped("Recover opens a dirty editor buffer. Review it, then Save to replace the authored file. Autosaves are retained until you discard them.");
		ImGui::BeginChild("Recovery files", { 0.f, -80.f });
		if (mRecoveryEntries.empty()) ImGui::TextDisabled("No recoverable autosaves.");
		for (std::size_t index = 0; index < mRecoveryEntries.size(); ++index)
		{
			const auto& entry = mRecoveryEntries[index];
			ImGui::PushID(static_cast<int>(index));
			ImGui::Separator();
			ImGui::TextWrapped("%s (%s)%s", entry.source.generic_string().c_str(), entry.kind.c_str(), entry.unclean ? " - unclean exit" : "");
			ImGui::TextDisabled("Session: %s", entry.snapshot.parent_path().filename().string().c_str());
			ImGui::TextWrapped("%s", entry.summary.c_str());
			ImGui::BeginDisabled(mContext.playMode || mContext.transitioning);
			if (ImGui::Button("Recover") && RecoverAutosave(entry))
			{
				mRecoveryEntries.erase(mRecoveryEntries.begin() + static_cast<std::ptrdiff_t>(index));
				ImGui::EndDisabled();
				ImGui::PopID();
				break;
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			if (ImGui::Button("Discard snapshot") && mAutosaves.Discard(entry))
			{
				mRecoveryEntries.erase(mRecoveryEntries.begin() + static_cast<std::ptrdiff_t>(index));
				ImGui::PopID();
				break;
			}
			ImGui::PopID();
		}
		ImGui::EndChild();
		if (!mAutosaveError.empty()) ImGui::TextWrapped("%s", mAutosaveError.c_str());
		if (ImGui::Button("Close")) ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}
}
