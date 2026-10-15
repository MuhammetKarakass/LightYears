#pragma once

#include "editor/EditorViewport.h"
#include "editor/EditorSelection.h"
#include "editor/EditorCommands.h"
#include "editor/EditorScript.h"
#include "editor/GameExporter.h"
#include "editor/JsonDocument.h"
#include "editor/EditorDiagnostics.h"
#include <map>
#include <framework/ProjectSettings.h>
#include <gameFramework/GameApplication.h>

namespace vega
{
	class EditorApplication : public ly::GameApplication
	{
	public:
		EditorApplication();
		~EditorApplication() override;
		int GetExitCode() const override;

	protected:
		bool HandleApplicationEvent(const sf::Event& event) override;
		void AfterRender() override;

	private:
		void Render() override;
		void BuildUI();
		void BuildToolbar();
		void BuildStatusBar();
		void BuildGizmo();
		void DrawWorldOverlays(sf::RenderTexture& texture, ly::World& world);
		void BuildViewportPanel();
		void BuildHierarchyPanel();
		void BuildInspectorPanel();
		void BuildStatsPanel();
		void BuildContentPanel();
		void BuildDataPanel();
			void BuildExportDialog();
		void DrawJsonNode(const Json& value, const std::string& pointer, const std::string& label);
		void DrawDirectory(const std::filesystem::path& relative, unsigned int depth = 0);
		void RequestDocument(const std::filesystem::path& relative);
		void FinishDocumentRequest();
		void RefreshContent();
		void PickViewportActor();
		bool OpenScene(const std::filesystem::path& path, bool allowMissing = false);
		bool SaveScene(const std::filesystem::path& path = {});
		bool SetPlayMode(bool playing);
		bool SceneDirty() const;
		void LoadSceneWorld(const nlohmann::json& scene, bool edit);
		void RequestScene(const std::string& action, const std::filesystem::path& path = {});
		void FinishSceneRequest();
		void BuildSceneDialogs();
		void BuildProjectDialog();
		void DrawCreateMenu();
		void BeginRename();
		void RequestPrefab(const std::filesystem::path& path);
		bool OpenPrefab(const std::filesystem::path& path);
		bool SavePrefab();
		bool PrefabDirty() const;
		void ClosePrefab();
		void BuildPrefabDialogs();
		void PrefabPropertyMenu(ly::Actor&, const ReflectedTarget&, const std::string& property);
		nlohmann::json CaptureScene() const;
		std::unique_ptr<ly::World> mPrefabWorld;
		EditorSelection mPrefabSelection;
		EditorContext mPrefabContext{ nullptr, mPrefabSelection };
		CommandHistory mPrefabHistory{ mPrefabContext };
		nlohmann::json mPrefabDocument, mPrefabSaved;
		mutable std::uint64_t mPrefabDirtyRevision = 0;
		mutable bool mPrefabDirty = false;
		std::filesystem::path mPrefabPath, mPendingPrefab;
		bool mConfirmPrefab = false;
		bool mPrefabCloseRequested = false;
		bool mCreatePrefabDialog = false;
		std::string mVariantParent;
		std::string mPrefabPendingSceneAction;
		std::filesystem::path mPrefabPendingScenePath;
		char mPrefabName[256]{};

		std::filesystem::path mScenePath;
		bool mProjectDialogRequested = false;
		ly::ProjectSettings mProjectDraft;
		int mProjectWindowSize[2]{ 1920, 1080 };
		std::string mProjectError;
		nlohmann::json mSceneSettings = nlohmann::json::object();
		std::uint64_t mSavedSceneRevision = 0;
		std::string mWindowTitle;
		nlohmann::json mPlaySnapshot;
		EditorCamera mEditCamera;
		std::string mEditSelectionGuid;
		std::string mRestoreSelectionGuid;
		bool mBaselineOnLoad = false;
		bool mOpenSceneDialog = false;
		bool mSaveSceneDialog = false;
		bool mConfirmScene = false;
		std::string mPendingSceneAction;
		std::filesystem::path mPendingScenePath;
		char mScenePathInput[1024]{ "scenes/sandbox.scene.json" };
		std::string mRenameGuid;
		char mRenameInput[512]{};
		bool mRenameFocus = false;
		std::optional<unsigned int> mHierarchyPopupActor;
		bool mDataFocused = false;

		EditorViewport mViewport;
		EditorDiagnostics mDiagnostics;
		EditorSelection mSelection;
		EditorCamera mCamera;
		JsonDocument mDocument{ VEGA_SOURCE_ASSETS_DIR };
		EditorContext mContext{ nullptr, mSelection, &mCamera };
		CommandHistory mHistory{ mContext };
		std::unique_ptr<EditorScript> mScript;
			// Build-tree locations are compile definitions of this target; see VegaEditor/CMakeLists.txt.
			GameExportLayout mExportLayout{ VEGA_GAME_BUILD_DIR, VEGA_GAME_VISUAL_ASSETS_DIR, VEGA_SOURCE_ROOT, VEGA_BUILD_ROOT };
			// The worker owns copies of its inputs and never accesses UI or world state.
			bool mExportDialogRequested = false;
			std::future<GameExportResult> mExportFuture;
			char mExportDirectory[512]{ VEGA_EXPORT_DEFAULT_DIR };
			bool mExportOverwrite = false;
			bool mExportSmokeTest = true;
			std::optional<GameExportResult> mExportResult;
		std::optional<unsigned int> mWorldID;
		std::optional<unsigned int> mMoveDragActor;
		std::optional<unsigned int> mDestroyActor;
		sf::Vector2f mMoveDragMouse{};
		sf::Vector2f mGizmoStartLocation{};
		float mGizmoStartRotation = 0.f;
		float mGizmoPreviousAngle = 0.f;
		float mGizmoRotationDelta = 0.f;
		GizmoHandle mGizmoHandle = GizmoHandle::None;
		bool mGizmoDragStarted = false;
		bool mPauseAfterTick = false;
		std::uint64_t mMoveMergeGroup = 0;
		std::uint64_t mPropertyMergeGroup = 0;
		sf::Clock mUIClock;
		sf::FloatRect mViewportImageRect{};
		char mHierarchyFilter[128]{};
		bool mViewportImageHovered = false;
		bool mViewportWindowFocused = false;
		bool mConsumeSelectionClick = false;
		bool mSelectionClickPending = false;
		std::optional<sf::Mouse::Button> mCameraPanButton;
		sf::Vector2i mCameraPanPixel{};
		bool mConsumeFocusKey = false;
		bool mDockLayoutInitialized = false;
		bool mShowViewport = true;
		bool mShowHierarchy = true;
		bool mShowInspector = true;
		bool mShowStats = true;
		bool mShowContent = true;
		bool mShowData = false;
		bool mConfirmDocument = false;
		bool mPendingClose = false;
		bool mPendingQuit = false;
		std::filesystem::path mPendingDocument;
		std::filesystem::path mContentDirectory;
		std::vector<std::filesystem::directory_entry> mContentEntries;
		std::string mContentError;
		char mContentFilter[128]{};
		float mContentTileSize = 88.f;
		std::uint64_t mJsonMergeGroup = 0;
		std::uint64_t mPresentedDocumentID = 0;
		struct Thumbnail
		{
			std::filesystem::file_time_type modified;
			std::unique_ptr<sf::Texture> texture;
		};
		std::map<std::filesystem::path, Thumbnail> mThumbnails;
		// ImGui consumes texture IDs after all panels have finished building this frame.
		std::vector<std::unique_ptr<sf::Texture>> mRetiredThumbnails;
	};
}
