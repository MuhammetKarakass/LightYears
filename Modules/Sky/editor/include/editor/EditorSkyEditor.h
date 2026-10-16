#pragma once

#include "editor/SkyDocument.h"
#include "editor/EditorScript.h"
#include <sky/SkyEditorHost.h>
#include <editor/EditorTool.h>
#include <editor/EditorExtensions.h>
#include <map>

namespace vega
{
	class EditorSkyEditor : public EditorModuleTool
	{
	public:
		explicit EditorSkyEditor(std::shared_ptr<sky::SkyEditorHost> host);
		void Register(EditorExtensions&, const std::shared_ptr<EditorSkyEditor>&);
		void ViewMenu() override;
		const char* Name() const override { return "Sky"; }
		void Update(EditorContext& scene, sf::Vector2u output, bool suspended) override;
		void Build(EditorContext& scene, CommandHistory& sceneHistory);
		void Handles(const sf::FloatRect& image, sf::Vector2u textureSize, bool hovered);
		bool Execute(const EditorScriptCommand& command, EditorContext& scene, std::string& detail);
		bool Active() const override { return visible && mMode && !mSuspended && mDocument && mDocument->document.IsOpen(); }
		bool Dirty() const override { return mDocument && mDocument->document.Dirty(); }
		CommandHistory* History() override { return Active() ? &mDocument->history : nullptr; }
		void RequestExit(std::function<void()> continuation) override;
		bool visible = false;
	private:
		bool Open(const std::string& stop, bool reopen = false);
		void EndPreview(EditorContext& scene);
		void ApplyPreview(bool force = false);
		bool Drag(const std::string& handle, sf::Vector2f delta, std::uint64_t group = 0);
		void CacheRows();
		void DrawFields(const std::string& block);
		void BlockTools(const std::string& path);
		bool RenderSky(sf::RenderTexture& target, std::size_t index, const ly::World& world, sf::Vector2u size);
		void Thumbnails(EditorContext& scene);
		std::shared_ptr<sky::SkyEditorHost> mPipeline;
		std::unique_ptr<SkyDocument> mDocument;

		sf::Vector2u mOutput{1920, 1080};
		sky::SkyGeometry mGeometry;
		struct Row { const sky::SkyField* field; std::string path; Json value; bool authored; std::string warning; };
		std::vector<Row> mRows;
		std::vector<std::pair<std::string, std::string>> mUnknown;
		Json mEffective;
		std::string mRowsSelection;
		std::uint64_t mRowsRevision = 0, mObservedRevision = 0, mAppliedRevision = 0, mGroup = 0;
		std::chrono::steady_clock::time_point mEditTime{};
		bool mMode = false, mSuspended = false, mPreviewing = false, mCompare = false, mAppliedCompare = false, mRebuilding = false;
		bool mConfirmClose = false, mWasVisible = false;
		bool mFocusRequested = false;
		bool mWasFocused = false;
		bool mRefreshGeometryRows = false;
		std::function<void()> mAfterClose;
		bool mSnap = false;
		std::string mDragHandle;
		sf::Vector2f mDragPrevious;
		sf::Vector2f mDragStart{}, mDragTotal{};
		Json mClipboard;
		std::string mClipboardKind;
		std::size_t mDuplicateTarget = 0;
		std::size_t mSkyIndex = 0;
		std::optional<std::size_t> mSceneSkyIndex;
		char mPresetName[128]{};
		struct Thumbnail { std::unique_ptr<sf::RenderTexture> target; std::uint64_t revision = 0; };
		std::map<std::string, Thumbnail> mThumbnails;
		double mApplyMs = 0, mRenderMs = 0, mThumbnailMs = 0;
	};
}
