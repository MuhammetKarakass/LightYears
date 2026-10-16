#include "editor/EditorApplication.h"
#include <level/SceneLevel.h>
#include <framework/scene/Scene.h>
#include <framework/scene/SceneEntityComponent.h>
#include <fstream>
#include <algorithm>
#include <cstdio>
#include <imgui.h>
#include "editor/PropertyWidgets.h"
#include "editor/EditorPrefabs.h"

namespace vega
{
	void EditorApplication::BuildProjectDialog()
	{
		if (mProjectDialogRequested)
		{
			mProjectDialogRequested = false;
			mProjectError.clear();
			try
			{
				const auto path = mDocument.Root() / "vega.project.json";
				mProjectDraft = std::filesystem::exists(path) ? ly::ProjectSettings::Load(path) : ly::ProjectSettings{};
				mProjectWindowSize[0] = static_cast<int>(mProjectDraft.windowWidth);
				mProjectWindowSize[1] = static_cast<int>(mProjectDraft.windowHeight);
			}
			catch (const std::exception& error) { mProjectError = error.what(); }
			ImGui::OpenPopup("Project Settings");
		}
		ImGui::SetNextWindowSize({ 560.f, 0.f }, ImGuiCond_Appearing);
		if (!ImGui::BeginPopupModal("Project Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) return;
		InputStringProperty("Name", mProjectDraft.name);
		if (ImGui::BeginCombo("Startup scene", mProjectDraft.startupScene.empty() ? "Default level" : mProjectDraft.startupScene.c_str()))
		{
			if (ImGui::Selectable("Default level", mProjectDraft.startupScene.empty())) mProjectDraft.startupScene.clear();
			std::error_code error;
			std::vector<std::string> scenes;
			for (std::filesystem::recursive_directory_iterator it{ mDocument.Root() / "scenes", error }, end; it != end && !error; it.increment(error))
			{
				const auto path = it->path().lexically_relative(mDocument.Root()).generic_string();
				if (it->is_regular_file(error) && path.size() >= 11 && path.substr(path.size() - 11) == ".scene.json") scenes.push_back(path);
			}
			std::sort(scenes.begin(), scenes.end());
			for (const auto& path : scenes)
				if (ImGui::Selectable(path.c_str(), path == mProjectDraft.startupScene)) mProjectDraft.startupScene = path;
			ImGui::EndCombo();
		}
		ImGui::InputInt2("Window size", mProjectWindowSize);
		ImGui::TextDisabled("Window changes apply the next time the game starts.");
		if (ImGui::Button("Save"))
		{
			try
			{
				mProjectDraft.windowWidth = static_cast<unsigned int>(mProjectWindowSize[0]);
				mProjectDraft.windowHeight = static_cast<unsigned int>(mProjectWindowSize[1]);
				const auto project = ly::ProjectSettings::Parse(mProjectDraft.ToJson());
				if (AtomicWriteText(mDocument.Root() / "vega.project.json", project.ToJson().dump(2) + "\n", mProjectError)) ImGui::CloseCurrentPopup();
			}
			catch (const std::exception& error) { mProjectError = error.what(); }
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
		if (!mProjectError.empty()) ImGui::TextWrapped("%s", mProjectError.c_str());
		ImGui::EndPopup();
	}

	namespace
	{
		nlohmann::json EmptyScene()
		{
			return { { "format", "vega.scene" }, { "version", 1 }, { "settings", nlohmann::json::object() },
				{ "entities", nlohmann::json::array() } };
		}
	}

	nlohmann::json EditorApplication::CaptureScene() const
	{
		return mContext.playMode ? mPlaySnapshot : mContext.world
			? ly::SceneSerializer::Save(*mContext.world, mSceneSettings) : EmptyScene();
	}

	bool EditorApplication::SceneDirty() const
	{
		return !mBaselineOnLoad && mContext.sceneRevision != mSavedSceneRevision;
	}

	void EditorApplication::LoadSceneWorld(const nlohmann::json& scene, bool edit)
	{
		mSettingsPanelRevision.reset();
		mContext.settingError.clear();
		mMoveDragActor.reset();
		mMarquee = false;
		mRenameGuid.clear();
		mPauseAfterTick = false;
		mContext.transitioning = true;
		LoadWorld<ly::SceneLevel>(scene, [this](const nlohmann::json& settings) { ApplySceneSettings(settings); }, edit);
	}

	bool EditorApplication::OpenScene(const std::filesystem::path& requested, bool allowMissing)
	{
		if (mContext.playMode) return false;
		try
		{
			const auto path = requested.is_absolute() ? requested : mDocument.Root() / requested;
			nlohmann::json scene = EmptyScene();
			if (std::ifstream input{ path }; input) input >> scene;
			else if (!allowMissing) throw std::runtime_error("Cannot open scene: " + path.string());
			if (!scene.is_object() || scene.value("format", std::string{}) != "vega.scene" ||
				scene.value("version", 0) != 1 || !scene.contains("entities") || !scene["entities"].is_array() ||
				!scene.contains("settings") || !scene["settings"].is_object()) throw std::runtime_error("Expected vega.scene version 1");
			const auto absolutePath = std::filesystem::absolute(path).lexically_normal();
			const bool differentScene = absolutePath != mScenePath;
			EditorSelection selected;
			if (!differentScene) selected = mSelection;
			mScenePath = absolutePath;
			mSceneSettings = scene["settings"];
			if (differentScene) mHistory.Clear();
			mSelection.Clear();
			mEditSelection.Clear();
			mRestoreSelection = selected;
			mBaselineOnLoad = true;
			mContext.sceneError.clear();
			LoadSceneWorld(scene, true);
			return true;
		}
		catch (const std::exception& error) { mContext.sceneError = error.what(); return false; }
	}

	bool EditorApplication::SaveScene(const std::filesystem::path& requested)
	{
		if (mContext.transitioning) return false;
		try
		{
			auto path = requested.empty() ? mScenePath : requested;
			if (path.empty())
			{
				std::snprintf(mScenePathInput, sizeof(mScenePathInput), "%s", "scenes/untitled.scene.json");
				mSaveSceneDialog = true;
				return false;
			}
			if (path.is_relative()) path = mDocument.Root() / path;
			path = std::filesystem::absolute(path).lexically_normal();
			const auto scene = CaptureScene();
			if (!AtomicWriteText(path, scene.dump(1, '\t') + "\n", mContext.sceneError)) return false;
			mScenePath = path;
			mSavedSceneRevision = mContext.sceneRevision;
			return true;
		}
		catch (const std::exception& error) { mContext.sceneError = error.what(); return false; }
	}

	bool EditorApplication::SetPlayMode(bool playing)
	{
		if (playing && PrefabDirty())
		{
			mPrefabPendingSceneAction = "play"; mConfirmPrefab = true;
			return false;
		}
		if (playing) ClosePrefab();
		if (!mContext.world || mContext.transitioning || playing == mContext.playMode) return false;
		mGasDebugger.Clear();
		if (playing) sas::AbilitySystemDebugView::BeginPlay();
		else sas::AbilitySystemDebugView::recording = false;
		if (playing)
		{
			if (mDiagnostics.clearOnPlay) mDiagnostics.Clear();
			mPlaySnapshot = CaptureScene();
			mEditCamera = mCamera;
			mEditSelection = mSelection;
		}
		mRestoreSelection = mEditSelection;
		mContext.playMode = playing;
		if (!playing) mCamera = mEditCamera;
		LoadSceneWorld(mPlaySnapshot, !playing);
		return true;
	}

	void EditorApplication::RequestScene(const std::string& action, const std::filesystem::path& path)
	{
		if (action == "exit" && mContentAuthoring.Dirty())
		{
			mContentAuthoring.RequestExit([this]() { RequestScene("exit"); });
			return;
		}
		if (PrefabDirty())
		{
			mPrefabPendingSceneAction = action; mPrefabPendingScenePath = path; mConfirmPrefab = true;
			return;
		}
		ClosePrefab();
		if (mContext.transitioning || (mContext.playMode && action != "exit")) return;
		if (action == "play") { SetPlayMode(true); return; }
		mPendingSceneAction = action;
		mPendingScenePath = path;
		if (SceneDirty()) mConfirmScene = true;
		else FinishSceneRequest();
	}

	void EditorApplication::FinishSceneRequest()
	{
		const auto action = mPendingSceneAction;
		const auto path = mPendingScenePath;
		mPendingSceneAction.clear();
		mPendingScenePath.clear();
		if (action == "exit")
		{
			if (mDocument.Dirty()) { mPendingQuit = true; mConfirmDocument = true; }
			else QuitApplication();
		}
		else if (action == "open") OpenScene(path);
		else if (action == "new")
		{
			mHistory.Clear();
			mSelection.Clear();
			mScenePath.clear();
			mSceneSettings = nlohmann::json::object();
			mRestoreSelection.Clear();
			mEditSelection.Clear();
			mBaselineOnLoad = true;
			mContext.sceneError.clear();
			LoadSceneWorld(EmptyScene(), true);
		}
	}

	void EditorApplication::BuildSceneDialogs()
	{
		if (mOpenSceneDialog) { ImGui::OpenPopup("Open Scene"); mOpenSceneDialog = false; }
		if (mSaveSceneDialog) { ImGui::OpenPopup("Save Scene As"); mSaveSceneDialog = false; }
		for (bool save : { false, true })
		{
			ImGui::SetNextWindowSize({ 600.f, 0.f }, ImGuiCond_Appearing);
			if (!ImGui::BeginPopupModal(save ? "Save Scene As" : "Open Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) continue;
			if (!save)
			{
				std::error_code error;
				std::vector<std::filesystem::path> scenes;
				for (std::filesystem::recursive_directory_iterator it{ mDocument.Root() / "scenes", error }, end;
					!error && it != end; it.increment(error))
				{
					const auto name = it->path().filename().string();
					if (it->is_regular_file(error) && name.size() >= 11 && name.compare(name.size() - 11, 11, ".scene.json") == 0)
						scenes.push_back(it->path().lexically_relative(mDocument.Root()));
				}
				std::sort(scenes.begin(), scenes.end());
				ImGui::BeginChild("Scenes", { 570.f, 180.f }, ImGuiChildFlags_Borders);
				for (const auto& path : scenes)
					if (ImGui::Selectable(path.generic_string().c_str())) std::snprintf(mScenePathInput, sizeof(mScenePathInput), "%s", path.generic_string().c_str());
				ImGui::EndChild();
			}
			ImGui::SetNextItemWidth(570.f);
			ImGui::InputText("Path", mScenePathInput, sizeof(mScenePathInput));
			if (!mContext.sceneError.empty()) ImGui::TextWrapped("%s", mContext.sceneError.c_str());
			if (ImGui::Button(save ? "Save" : "Open"))
			{
				if (save)
				{
					if (SaveScene(std::filesystem::u8path(mScenePathInput)))
					{
						ImGui::CloseCurrentPopup();
						if (!mPendingSceneAction.empty()) FinishSceneRequest();
					}
				}
				else { RequestScene("open", std::filesystem::u8path(mScenePathInput)); ImGui::CloseCurrentPopup(); }
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel")) { mPendingSceneAction.clear(); ImGui::CloseCurrentPopup(); }
			ImGui::EndPopup();
		}
		if (mConfirmScene) { ImGui::OpenPopup("Unsaved scene changes"); mConfirmScene = false; }
		if (ImGui::BeginPopupModal("Unsaved scene changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::TextUnformatted("Save scene changes before continuing?");
			if (!mContext.sceneError.empty()) ImGui::TextWrapped("%s", mContext.sceneError.c_str());
			if (ImGui::Button("Save"))
			{
				if (SaveScene()) { FinishSceneRequest(); ImGui::CloseCurrentPopup(); }
				else if (mSaveSceneDialog) ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Discard")) { FinishSceneRequest(); ImGui::CloseCurrentPopup(); }
			ImGui::SameLine();
			if (ImGui::Button("Cancel")) { mPendingSceneAction.clear(); ImGui::CloseCurrentPopup(); }
			ImGui::EndPopup();
		}
	}

	void EditorApplication::DrawCreateMenu()
	{
		if (ImGui::BeginMenu("Create"))
		{
			if (ImGui::BeginMenu("Prefabs"))
			{
				for (const auto& path : PrefabPaths(mDocument.Root()))
					if (ImGui::MenuItem(path.c_str())) PlacePrefab(mContext, mHistory, path, mContext.world->GetWorldView().getCenter());
				ImGui::EndMenu();
			}
			auto types = ly::reflect::TypeRegistry::Get().SpawnableActors();
			std::sort(types.begin(), types.end(), [](const auto* a, const auto* b) { return a->name < b->name; });
			for (const auto* type : types)
				if (ImGui::MenuItem(type->name.c_str())) CreateSceneEntity(mContext, mHistory, type->name);
			ImGui::EndMenu();
		}
	}

	void EditorApplication::BeginRename()
	{
		const auto actor = mSelection.Get();
		const auto* identity = actor ? actor->FindComponent<ly::SceneEntityComponent>() : nullptr;
		if (!identity || mContext.playMode) return;
		mRenameGuid = identity->guid;
		std::snprintf(mRenameInput, sizeof(mRenameInput), "%s", identity->name.c_str());
		mRenameFocus = true;
	}
}
