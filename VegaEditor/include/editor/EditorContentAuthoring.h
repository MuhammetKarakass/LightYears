#pragma once
#include "editor/ContentWorkspace.h"
#include "editor/EditorScript.h"
#include <imgui.h>

namespace vega
{
	class EditorContentAuthoring
	{
	public:
		explicit EditorContentAuthoring(vega::EngineContext& context = vega::EngineContext::Current()) : mWorkspace{ context } {}
		bool visible = false;
		void Build(EditorContext&, CommandHistory&, const std::filesystem::path& root);
		bool Execute(EditorContext&, CommandHistory&, const std::filesystem::path&, const EditorScriptCommand&, std::string& detail);
		bool Dirty() const { return mWorkspace.Dirty(); }
		bool Save() { return mWorkspace.Save(); }
		std::map<std::filesystem::path, std::string> DirtyDocuments() const { return mWorkspace.DirtyDocuments(); }
		bool Recover(const std::filesystem::path& root, const std::filesystem::path& path, const std::string& text)
		{
			visible = true;
			return (mWorkspace.Loaded() || mWorkspace.Load(root)) && mWorkspace.Recover(path, text);
		}
		const std::string& Error() const { return mWorkspace.error; }
		void ReloadIfLoaded(const std::filesystem::path& root) { if (mWorkspace.Loaded()) mWorkspace.Load(root); }
		void RequestExit(std::function<void()> resume) { visible = true; mExit = std::move(resume); mConfirmExit = true; }
	private:
		void DrawObject(const Json&, const std::string& path, const std::string& pattern, CommandHistory&);
		void DrawField(const Json&, const std::string& path, const std::string& pattern, const std::string& label, CommandHistory&);
		void RefreshCache();
		Json Default(const std::string& pattern) const;
		ContentWorkspace mWorkspace;
		ImGuiTextFilter mFilter, mTagFilter, mPickerFilter;
		std::map<std::string, std::vector<std::string>> mOptions;
		std::map<std::string, std::string> mOther;
		std::string mCachedSelection, mJumpType, mJumpId, mJumpPath;
		std::uint64_t mCachedRevision = ~0ull, mMergeGroup = 0;
		char mNewId[512]{}, mSource[512]{};
		std::string mOperation;
		bool mShowReport = false, mShowReferences = false;
		struct PendingEdit { std::string path; Json value; std::uint64_t group; };
		std::optional<PendingEdit> mPendingEdit;
		std::function<void()> mExit;
		bool mConfirmExit = false;
		bool mFocus = false;
	};
}
