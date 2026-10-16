#include "editor/EditorDiagnostics.h"
#include <imgui_internal.h>
#include <algorithm>
#include <cstdio>
#include <sstream>
#include <cstring>

namespace vega
{
	using namespace ly::debug;
	namespace
	{
		int Group(LogLevel level) { return level >= LogLevel::Error ? 2 : level == LogLevel::Warning ? 1 : 0; }
		ImVec4 Colour(LogLevel level)
		{
			return Group(level) == 2 ? ImVec4{ 1.f, .38f, .34f, 1.f } : Group(level) == 1
				? ImVec4{ 1.f, .76f, .28f, 1.f } : ImVec4{ .45f, .81f, .91f, 1.f };
		}
	}

	void EditorDiagnostics::InstallSettings()
	{
		ImGuiSettingsHandler handler;
		handler.TypeName = "VegaDiagnostics";
		handler.TypeHash = ImHashStr(handler.TypeName);
		handler.UserData = this;
		handler.ReadOpenFn = [](ImGuiContext*, ImGuiSettingsHandler* h, const char*) -> void* { return h->UserData; };
		handler.ReadLineFn = [](ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line)
		{
			auto& self = *static_cast<EditorDiagnostics*>(entry);
			if (std::strncmp(line, "Visible=", 8) != 0) return;
			std::istringstream input{ line + 8 };
			int console, profiler, clear;
			char first, second;
			if ((input >> console >> first >> profiler >> second >> clear) && first == ',' && second == ',')
			{
				self.showConsole = console != 0;
				self.showProfiler = profiler != 0;
				self.clearOnPlay = clear != 0;
			}
		};
		handler.WriteAllFn = [](ImGuiContext*, ImGuiSettingsHandler* h, ImGuiTextBuffer* out)
		{
			const auto& self = *static_cast<EditorDiagnostics*>(h->UserData);
			out->appendf("[VegaDiagnostics][Panels]\nVisible=%d,%d,%d\n\n", self.showConsole, self.showProfiler, self.clearOnPlay);
		};
		ImGui::AddSettingsHandler(&handler);
	}

	void EditorDiagnostics::TrackSettings()
	{
		const unsigned int flags = (showConsole ? 1u : 0u) | (showProfiler ? 2u : 0u) | (clearOnPlay ? 4u : 0u);
		if (flags != mSavedFlags) { mSavedFlags = flags; ImGui::MarkIniSettingsDirty(); }
	}

	void EditorDiagnostics::ReportError(const char* field, const std::string& error)
	{
		auto& previous = mEditorErrors[field];
		if (previous == error) return;
		previous = error;
		if (!error.empty()) Log::Write(LogLevel::Error, LogChannel::Editor, {}, std::string{ field } + ": " + error);
	}

	void EditorDiagnostics::Clear()
	{
		capture.Buffer().Clear();
		mSelected.reset();
		mLatest.clear();
		mLatestUntil = 0;
	}

	void EditorDiagnostics::RefreshConsole()
	{
		const auto state = capture.Buffer().State();
		std::copy(state.counts.begin(), state.counts.end(), mCounts);
		if (state.latestIssue && state.latestIssue->id > mLatestID)
		{
			mLatestID = state.latestIssue->id;
			mLatest = state.latestIssue->message;
			mLatestUntil = ImGui::GetTime() + 6.0;
		}
		// A closed Console needs only O(1) badge state; refresh rows when it opens again.
		if (!showConsole || mRevision == state.revision) return;
		mRevision = state.revision;
		mRecords = capture.Buffer().SharedSnapshot();
		FilterRows();
	}

	void EditorDiagnostics::FilterRows()
	{
		mRows.clear();
		mRows.reserve(mRecords.size());
		// Collapse before filtering so hidden records still break consecutive runs.
		const ConsoleRecord* previous = nullptr;
		bool included = false;
		for (const auto& shared : mRecords)
		{
			const auto& record = *shared;
			if (mCollapse && previous && LogBuffer::Identical(*previous, record))
			{
				if (included) ++mRows.back().count;
				continue;
			}
			previous = &record;
			included = mLevels[Group(record.level)] && (!mChannel || static_cast<int>(record.channel) == mChannel - 1) &&
				mSearch.PassFilter(record.message.c_str());
			if (included) mRows.push_back({ shared, 1 });
		}
	}

	void EditorDiagnostics::BuildStatus()
	{
		RefreshConsole();
		ImGui::SameLine();
		const std::string label = "Errors: " + std::to_string(mCounts[2]) +
			(ImGui::GetTime() < mLatestUntil ? "  " + mLatest.substr(0, 65) : "") + "##ConsoleStatus";
		if (ImGui::SmallButton(label.c_str())) { showConsole = true; mFocusConsole = true; }
		if (!mLatest.empty()) ImGui::SetItemTooltip("%s", mLatest.c_str());
	}

	void EditorDiagnostics::BuildConsole()
	{
		LY_PROFILE_SCOPE("Editor Console");
		const auto previousRevision = mDisplayedRevision;
		RefreshConsole();
		if (mFocusConsole) { ImGui::SetNextWindowFocus(); mFocusConsole = false; }
		if (ImGui::Begin("Console", &showConsole))
		{
			mDisplayedRevision = mRevision;
			if (ImGui::SmallButton("Clear")) { Clear(); RefreshConsole(); }
			ImGui::SameLine(); ImGui::Checkbox("Clear on Play", &clearOnPlay);
			ImGui::SameLine(); bool dirty = ImGui::Checkbox("Collapse", &mCollapse);
			ImGui::SameLine(); ImGui::Checkbox("Auto-scroll", &mAutoScroll);
			const char* levels[]{ "Info", "Warning", "Error" };
			for (int i = 0; i < 3; ++i)
			{
				if (i) ImGui::SameLine();
				const auto label = std::string{ levels[i] } + " (" + std::to_string(mCounts[i]) + ")";
				dirty |= ImGui::Checkbox(label.c_str(), &mLevels[i]);
			}
			ImGui::SetNextItemWidth(115.f);
			dirty |= ImGui::Combo("Channel", &mChannel, "All\0CORE\0GAME\0Lua\0Scene\0Editor\0");
			ImGui::SameLine(); dirty |= mSearch.Draw("Search", std::max(70.f, ImGui::GetContentRegionAvail().x - 55.f));
			if (dirty) FilterRows();
			const float detailsHeight = mSelected ? 90.f : 0.f;
			if (ImGui::BeginTable("Records", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY,
				{ 0.f, std::max(60.f, ImGui::GetContentRegionAvail().y - detailsHeight) }))
			{
				ImGui::TableSetupScrollFreeze(0, 1);
				ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 90.f);
				ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_WidthFixed, 75.f);
				ImGui::TableSetupColumn("Channel", ImGuiTableColumnFlags_WidthFixed, 65.f);
				ImGui::TableSetupColumn("Message");
				ImGui::TableHeadersRow();
				const bool atBottom = ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 2.f;
				ImGuiListClipper clipper;
				clipper.Begin(static_cast<int>(mRows.size()));
				while (clipper.Step()) for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
				{
					const auto& row = *mRows[i].record;
					ImGui::PushID(static_cast<int>(row.id));
					ImGui::TableNextRow(); ImGui::TableNextColumn();
					if (ImGui::Selectable(row.timestamp.c_str(), mSelected && mSelected->id == row.id, ImGuiSelectableFlags_SpanAllColumns)) mSelected = row;
					if (ImGui::BeginPopupContextItem("Copy"))
					{
						if (ImGui::MenuItem("Copy row")) ImGui::SetClipboardText((row.timestamp + " [" + Log::GetLevelName(row.level) + "] [" +
							Log::GetChannelName(row.channel) + "] " + row.message + "\n" + row.file + ":" + std::to_string(row.line)).c_str());
						ImGui::EndPopup();
					}
					ImGui::TableNextColumn(); ImGui::TextColored(Colour(row.level), "%s", Log::GetLevelName(row.level));
					ImGui::TableNextColumn(); ImGui::TextUnformatted(Log::GetChannelName(row.channel));
					ImGui::TableNextColumn();
					const auto firstLine = row.message.substr(0, row.message.find('\n'));
					if (mRows[i].count > 1) { ImGui::TextDisabled("[%zu]", mRows[i].count); ImGui::SameLine(); }
					ImGui::TextUnformatted(firstLine.c_str());
					ImGui::PopID();
				}
				if (mAutoScroll && atBottom && previousRevision != mRevision) ImGui::SetScrollHereY(1.f);
				ImGui::EndTable();
			}
			if (mSelected)
			{
				ImGui::BeginChild("Details", { 0.f, 0.f });
				ImGui::TextWrapped("%s:%d  %s", mSelected->file.c_str(), mSelected->line, mSelected->function.c_str());
				ImGui::TextWrapped("%s", mSelected->message.c_str());
				ImGui::EndChild();
			}
		}
		ImGui::End();
	}

	void EditorDiagnostics::BuildProfiler()
	{
		LY_PROFILE_SCOPE("Editor Profiler");
		if (ImGui::Begin("Profiler", &showProfiler))
		{
			bool recording = Profiler::IsEnabled();
			if (ImGui::Button(recording ? "Pause" : "Record")) { Profiler::SetEnabled(!recording); recording = !recording; }
			ImGui::SameLine();
			if (ImGui::Button("Capture to file"))
			{
				const auto path = std::filesystem::path{ VEGA_BUILD_ROOT } / "Saved" / "profile.json";
				if (Profiler::CaptureToFile(path, mCaptureStatus)) mCaptureStatus = path.string();
				else Log::Write(LogLevel::Error, LogChannel::Editor, LY_SOURCE_LOCATION, mCaptureStatus);
			}
			ImGui::SameLine(); ImGui::TextDisabled("%s", Profiler::IsCompiledIn() ? "300 frame window" : "Compiled out");
			const auto times = Profiler::FrameTimes();
			if (recording || !mProfile.id) mProfile = Profiler::GetFrame();
			const ImVec2 origin = ImGui::GetCursorScreenPos();
			const ImVec2 size{ std::max(100.f, ImGui::GetContentRegionAvail().x), 85.f };
			ImGui::InvisibleButton("Frames (click to inspect)", size);
			auto* draw = ImGui::GetWindowDrawList();
			draw->AddRectFilled(origin, { origin.x + size.x, origin.y + size.y }, IM_COL32(20, 25, 30, 255));
			float maximum = 40.f;
			for (const auto& time : times) maximum = std::max(maximum, time.second);
			maximum = std::min(maximum, 100.f);
			for (float guide : { 16.6f, 33.3f })
			{
				const float y = origin.y + size.y * (1.f - guide / maximum);
				draw->AddLine({ origin.x, y }, { origin.x + size.x, y }, IM_COL32(125, 115, 65, 170));
				draw->AddText({ origin.x + 4.f, y - 14.f }, IM_COL32(220, 210, 160, 255), guide < 20.f ? "16.6 ms" : "33.3 ms");
			}
			for (std::size_t i = 0; i < times.size(); ++i)
			{
				const float x = origin.x + size.x * static_cast<float>(i) / times.size();
				const float y = origin.y + size.y * (1.f - std::min(times[i].second, maximum) / maximum);
				draw->AddLine({ x, origin.y + size.y }, { x, y }, times[i].first == mProfile.id ? IM_COL32(255, 200, 70, 255) : IM_COL32(50, 175, 180, 210));
			}
			if (ImGui::IsItemHovered() && !times.empty())
			{
				const auto index = std::min(times.size() - 1, static_cast<std::size_t>(std::max(0.f, (ImGui::GetIO().MousePos.x - origin.x) / size.x) * times.size()));
				ImGui::SetTooltip("Frame %llu: %.2f ms (click to pause)", static_cast<unsigned long long>(times[index].first), times[index].second);
				if (ImGui::IsItemClicked()) { Profiler::SetEnabled(false); mProfile = Profiler::GetFrame(times[index].first); }
			}
			ImGui::Text("Frame %llu  %.2f ms", static_cast<unsigned long long>(mProfile.id), mProfile.milliseconds);
			if (!mProfile.id) ImGui::TextDisabled("Press Record to capture scopes and counters.");
			int counterIndex = 0;
			for (const auto& counter : mProfile.counters)
			{
				ImGui::TextDisabled("%s %.0f", counter.first.c_str(), counter.second);
				if (++counterIndex % 3) ImGui::SameLine();
			}
			ImGui::NewLine();
			if (!mCaptureStatus.empty()) ImGui::TextWrapped("%s", mCaptureStatus.c_str());
			if (ImGui::BeginTable("Scopes", 6, ImGuiTableFlags_Sortable | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
			{
				const char* columns[]{ "Scope", "Calls", "Total ms", "Self ms", "Avg ms", "Max ms" };
				for (int i = 0; i < 6; ++i) ImGui::TableSetupColumn(columns[i],
					(i == 0 ? ImGuiTableColumnFlags_WidthStretch : ImGuiTableColumnFlags_WidthFixed) |
					(i == 2 ? ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortDescending : 0), i ? 66.f : 0.f);
				ImGui::TableSetupScrollFreeze(0, 1); ImGui::TableHeadersRow();
				const auto* specs = ImGui::TableGetSortSpecs();
				const int column = specs && specs->SpecsCount ? specs->Specs[0].ColumnIndex : 2;
				const bool ascending = specs && specs->SpecsCount && specs->Specs[0].SortDirection == ImGuiSortDirection_Ascending;
				std::sort(mProfile.scopes.begin(), mProfile.scopes.end(), [column, ascending](const auto& a, const auto& b)
				{
					const auto value = [column](const auto& s)
					{
						switch (column)
						{
						case 1: return static_cast<double>(s.callCount);
						case 3: return s.selfMicroseconds;
						case 4: return s.totalMicroseconds / s.callCount;
						case 5: return s.maximumMicroseconds;
						default: return s.totalMicroseconds;
						}
					};
					if (column == 0) return ascending ? a.name < b.name : b.name < a.name;
					return ascending ? value(a) < value(b) : value(b) < value(a);
				});
				for (const auto& scope : mProfile.scopes)
				{
					ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::TextUnformatted(scope.name.c_str());
					ImGui::TableNextColumn(); ImGui::Text("%llu", static_cast<unsigned long long>(scope.callCount));
					for (double value : { scope.totalMicroseconds, scope.selfMicroseconds, scope.totalMicroseconds / scope.callCount, scope.maximumMicroseconds })
					{ ImGui::TableNextColumn(); ImGui::Text("%.3f", value / 1000.0); }
				}
				ImGui::EndTable();
			}
		}
		ImGui::End();
	}
}
