#pragma once

#ifdef VEGA_EDITOR_WITH_SKY

#include "editor/SkyDocument.h"
#include "editor/EditorScript.h"
#include <presentation/visual/GameVisualPipeline.h>
#include <map>

namespace vega
{
	class EditorSkyEditor
	{
	public:
		void Bind(ly::WorldRenderPipeline* pipeline);
		void Update(EditorContext& scene, sf::Vector2u output, bool suspended);
		void Build(EditorContext& scene, CommandHistory& sceneHistory);
		void Handles(const sf::FloatRect& image, sf::Vector2u textureSize, bool hovered);
		bool Execute(const EditorScriptCommand& command, EditorContext& scene, std::string& detail);
		bool Active() const { return visible && mMode && !mSuspended && mDocument && mDocument->document.IsOpen(); }
		bool Dirty() const { return mDocument && mDocument->document.Dirty(); }
		CommandHistory* History() { return Active() ? &mDocument->history : nullptr; }
		void RequestExit(std::function<void()> continuation);
		bool visible = false;
	private:
		bool Open(const std::string& stop, bool reopen = false);
		void EndPreview(EditorContext& scene);
		void ApplyPreview(bool force = false);
		bool Drag(const std::string& handle, sf::Vector2f delta, std::uint64_t group = 0);
		void CacheRows();
		void DrawFields(const std::string& block);
		void BlockTools(const std::string& path);
		bool RenderSky(sf::RenderTexture& target, const ly::RouteStop& stop, const ly::World& world, sf::Vector2u size);
		void Thumbnails(EditorContext& scene);
		ly::GameVisualPipeline* mPipeline = nullptr;
		std::unique_ptr<SkyDocument> mDocument;
		std::unique_ptr<ly::visual::VisualPipeline> mPreviewPipeline;
		ly::ProceduralBackdrop mPreviewBackdrop;
		std::string mPreviewRenderedStop;
		sf::Vector2u mOutput{1920, 1080};
		ly::ProceduralBackdrop::SkyGeometry mGeometry;
		struct Row { const ly::visual::SkyField* field; std::string path; Json value; bool authored; std::string warning; };
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
		std::size_t mStopIndex = 0;
		std::optional<int> mSceneRouteIndex;
		char mPresetName[128]{};
		struct Thumbnail { std::unique_ptr<sf::RenderTexture> target; std::uint64_t revision = 0; };
		std::map<std::string, Thumbnail> mThumbnails;
		double mApplyMs = 0, mRenderMs = 0, mThumbnailMs = 0;
	};
}
#else
#include "editor/EditorScript.h"
#include <framework/WorldRenderPipeline.h>
namespace vega
{
	// Optional tools retain their call surface without importing their module.
	class EditorSkyEditor
	{
	public:
		bool visible = false;
		void Bind(ly::WorldRenderPipeline*) {}
		void Update(EditorContext&, sf::Vector2u, bool) {}
		void Build(EditorContext&, CommandHistory&) {}
		void Handles(const sf::FloatRect&, sf::Vector2u, bool) {}
		bool Execute(const EditorScriptCommand&, EditorContext&, std::string& detail)
		{
			detail = "Sky is not enabled for this editor";
			return false;
		}
		bool Active() const { return false; }
		bool Dirty() const { return false; }
		CommandHistory* History() { return nullptr; }
		void RequestExit(std::function<void()> continuation) { continuation(); }
	};
}
#endif
