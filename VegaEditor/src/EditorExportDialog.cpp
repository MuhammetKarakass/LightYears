#include "editor/EditorApplication.h"
#include "editor/EditorIcons.h"
#include "editor/PropertyWidgets.h"
#include "editor/EditorPrefabs.h"
#include <framework/World.h>
#include <framework/Actor.h>
#include <framework/scene/SceneEntityComponent.h>
#include <script/ScriptComponent.h>
#include <script/ScriptRuntime.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui-SFML.h>
#include <algorithm>
#include <cctype>
#include <typeinfo>
#include <typeindex>
#include <string_view>

namespace vega
{
	void EditorApplication::BuildExportDialog()
	{
		if (mExportFuture.valid() && mExportFuture.wait_for(std::chrono::seconds{ 0 }) == std::future_status::ready)
		{
			try { mExportResult = mExportFuture.get(); }
			catch (const std::exception& error) { mExportResult = GameExportResult{}; mExportResult->error = error.what(); }
		}
		if (mExportDialogRequested)
		{
			ImGui::OpenPopup("Export Game");
			mExportDialogRequested = false;
		}
		ImGui::SetNextWindowSize({ 560.f, 0.f }, ImGuiCond_Appearing);
		if (!ImGui::BeginPopupModal("Export Game", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) return;
		const bool idle = !mExportFuture.valid();
		ImGui::BeginDisabled(!idle);
		ImGui::TextUnformatted("Output folder");
		ImGui::SetNextItemWidth(420.f);
		ImGui::InputText("##ExportDirectory", mExportDirectory, sizeof(mExportDirectory));
		ImGui::Checkbox("Overwrite a previous export", &mExportOverwrite);
		ImGui::SetItemTooltip("Removes only the files that an earlier export recorded in vega_export_manifest.txt. Other files stay.");
		ImGui::Checkbox("Run smoke test after export", &mExportSmokeTest);
		ImGui::EndDisabled();

		ImGui::BeginDisabled(!idle || mExportDirectory[0] == '\0');
		if (ImGui::Button("Export"))
		{
			GameExportRequest request;
			request.outputDir = std::filesystem::u8path(mExportDirectory);
			request.overwrite = mExportOverwrite;
			request.runSmokeTest = mExportSmokeTest;
			mExportResult.reset();
			try { mExportFuture = ExportGameAsync(mExportLayout, request); }
			catch (const std::exception& error) { mExportResult = GameExportResult{}; mExportResult->error = error.what(); }
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::BeginDisabled(mExportFuture.valid());
		if (ImGui::Button("Close")) ImGui::CloseCurrentPopup();
		ImGui::EndDisabled();
		if (mExportFuture.valid())
		{
			ImGui::TextUnformatted("Exporting\xe2\x80\xa6");
			ImGui::ProgressBar(-static_cast<float>(ImGui::GetTime()), { -1.f, 0.f }, "");
		}
		if (mExportResult)
		{
			ImGui::Separator();
			const bool passed = mExportResult->success && (!mExportResult->smokeTestRan || mExportResult->smokeTestPassed);
			ImGui::PushStyleColor(ImGuiCol_Text, passed ? ImVec4{ 0.45f, 0.85f, 0.45f, 1.f } : ImVec4{ 0.95f, 0.45f, 0.40f, 1.f });
			ImGui::TextWrapped("%s", DescribeGameExport(*mExportResult).c_str());
			ImGui::PopStyleColor();
		}
		ImGui::EndPopup();
	}
}
