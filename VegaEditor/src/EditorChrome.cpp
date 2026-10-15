#include "editor/EditorApplication.h"
#include "editor/EditorIcons.h"
#include <framework/World.h>
#include <imgui_internal.h>
#include <cmath>

namespace vega
{
	void EditorApplication::BuildToolbar()
	{
		auto* viewport = ImGui::GetMainViewport();
		const auto flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDocking;
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 12.f, 7.f });
		if (ImGui::BeginViewportSideBar("##VegaToolbar", viewport, ImGuiDir_Up, 48.f, flags))
		{
			auto& settings = mContext.viewSettings;
			ly::World* world = GetCurrentWorld();
			const bool paused = world && world->IsPaused();
			if (!ImGui::GetIO().WantTextInput && !ImGui::GetIO().KeyCtrl && !mMoveDragActor)
			{
				if (ImGui::IsKeyPressed(ImGuiKey_W, false)) settings.tool = EditorTool::Move;
				if (ImGui::IsKeyPressed(ImGuiKey_E, false)) settings.tool = EditorTool::Rotate;
			}
			ImGui::AlignTextToFramePadding();
			ImGui::TextColored({ .20f, .78f, .79f, 1.f }, "VEGA");
			ImGui::SameLine(82.f);
			ImGui::BeginDisabled(mMoveDragActor.has_value());
			if (IconButton("##Move", IconKind::Move, { 34.f, 34.f }, "Move (W)", settings.tool == EditorTool::Move)) settings.tool = EditorTool::Move;
			ImGui::SameLine();
			if (IconButton("##Rotate", IconKind::Rotate, { 34.f, 34.f }, "Rotate (E)", settings.tool == EditorTool::Rotate)) settings.tool = EditorTool::Rotate;
			ImGui::EndDisabled();
			ImGui::SameLine();
			ImGui::BeginDisabled(!world || mContext.transitioning || mMoveDragActor.has_value());
			const bool editorCamera = world && world->GetViewOverride().has_value();
			if (IconButton("##Camera", IconKind::Camera, { 34.f, 34.f }, "Editor camera / Game camera\nWheel: zoom   Middle/right drag: pan   F: focus", editorCamera)) mContext.SetEditorCamera(!editorCamera);
			ImGui::EndDisabled();
			ImGui::SameLine();
			if (IconButton("##Snap", IconKind::Snap, { 34.f, 34.f }, "Snap to grid (or hold Ctrl)", settings.snap)) settings.snap = !settings.snap;
			ImGui::SameLine();
			if (ImGui::Button("v##SnapSettings", { 24.f, 34.f })) ImGui::OpenPopup("Snap settings");
			if (ImGui::BeginPopup("Snap settings"))
			{
				ImGui::TextUnformatted("Snapping");
				ImGui::SetNextItemWidth(150.f);
				ImGui::DragFloat("Grid units", &settings.gridSize, 1.f, 4.f, 512.f, "%.0f", ImGuiSliderFlags_AlwaysClamp);
				if (!std::isfinite(settings.gridSize)) settings.gridSize = 16.f;
				ImGui::TextDisabled("Rotation: 15 degrees");
				ImGui::EndPopup();
			}
			// Keep transport controls centred independently of the tools on the left.
			ImGui::SameLine(viewport->Size.x * .5f - 38.f);
			ImGui::BeginDisabled(!world || mMoveDragActor.has_value());
			if (!mContext.playMode)
			{
				if (IconButton("##Transport", IconKind::Play, { 34.f, 34.f }, "Play scene")) SetPlayMode(true);
			}
			else
			{
				if (ImGui::Button("Stop", { 50.f, 34.f })) SetPlayMode(false);
				ImGui::SameLine();
				if (IconButton("##Pause", paused ? IconKind::Play : IconKind::Pause, { 34.f, 34.f }, paused ? "Resume" : "Pause"))
					SetPaused{ !paused }.Execute(mContext);
			}
			ImGui::SameLine();
			ImGui::BeginDisabled(!mContext.playMode || !paused || mContext.transitioning);
			if (IconButton("##Step", IconKind::Step, { 34.f, 34.f }, "Advance one simulation frame", false))
			{
				// Render runs after the next world tick; pause there to include physics and timers.
				world->SetPaused(false);
				mPauseAfterTick = true;
			}
			ImGui::EndDisabled();
			ImGui::EndDisabled();
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextColored(mContext.playMode ? ImVec4{ 1.f, .65f, .25f, 1.f } : ImVec4{ .2f, .78f, .79f, 1.f },
				"%s", mContext.playMode ? (paused ? "PLAYING (PAUSED)" : "PLAYING") : "EDIT");
			ImGui::SameLine();
			ImGui::TextDisabled("%s%s", mScenePath.empty() ? "Untitled" : mScenePath.filename().string().c_str(), SceneDirty() ? " *" : "");
			if (!mContext.sceneError.empty()) { ImGui::SameLine(); ImGui::TextWrapped("%s", mContext.sceneError.c_str()); }
		}
		ImGui::End();
		ImGui::PopStyleVar();
	}

	void EditorApplication::BuildStatusBar()
	{
		const auto flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDocking;
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 12.f, 5.f });
		if (ImGui::BeginViewportSideBar("##VegaStatus", ImGui::GetMainViewport(), ImGuiDir_Down, 30.f, flags))
		{
			const auto actor = mSelection.Get();
			ImGui::Text("%s", actor ? (ActorTypeName(*actor) + "  #" + std::to_string(actor->GetUniqueID())).c_str() : "No selection");
			ImGui::SameLine();
			ImGui::TextDisabled(" |  %s  |  Snap %s (%.0f)  |  Undo %zu / Redo %zu",
				mContext.viewSettings.tool == EditorTool::Move ? "Move [W]" : "Rotate [E]",
				(mContext.viewSettings.snap || ImGui::GetIO().KeyCtrl) ? "on" : "off", mContext.viewSettings.gridSize,
				mHistory.UndoCount(), mHistory.RedoCount());
			ImGui::SameLine(ImGui::GetWindowWidth() - 94.f);
			ImGui::TextDisabled("%.0f FPS", ImGui::GetIO().Framerate);
		}
		ImGui::End();
		ImGui::PopStyleVar();
	}
}
