#pragma once

#include <framework/debug/LogBuffer.h>
#include <framework/debug/Profiler.h>
#include <imgui.h>
#include <optional>

namespace vega
{
	class EditorDiagnostics
	{
	public:
		void InstallSettings();
		void RefreshConsole();
		void BuildConsole();
		void BuildProfiler();
		void BuildStatus();
		void Clear();
		void FocusConsole() { showConsole = true; mFocusConsole = true; }
		void TrackSettings();
		void ReportError(const char* field, const std::string& error);
		vega::debug::LogCapture capture;
		bool showConsole = true, showProfiler = true, clearOnPlay = false;
	private:
		void FilterRows();
		bool mCollapse = true, mAutoScroll = true, mFocusConsole = false;
		bool mLevels[3]{ true, true, true };
		int mChannel = 0;
		ImGuiTextFilter mSearch;
		std::uint64_t mRevision = 0;
		std::uint64_t mDisplayedRevision = 0;
		std::unordered_map<std::string, std::string> mEditorErrors;
		struct ConsoleRow
		{
			std::shared_ptr<const vega::debug::ConsoleRecord> record;
			std::size_t count = 1;
		};
		std::vector<std::shared_ptr<const vega::debug::ConsoleRecord>> mRecords;
		std::vector<ConsoleRow> mRows;
		std::optional<vega::debug::ConsoleRecord> mSelected;
		std::size_t mCounts[3]{};
		std::string mLatest;
		double mLatestUntil = 0;
		std::uint64_t mLatestID = 0;
		unsigned int mSavedFlags = 0;
		vega::debug::ProfileFrame mProfile;
		std::string mCaptureStatus;
	};
}
