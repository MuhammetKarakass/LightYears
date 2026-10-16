#pragma once

#include "editor/EditorViewport.h"
#include "editor/EditorSelection.h"
#include "editor/EditorCommands.h"
#include "editor/EditorScript.h"
#include "editor/GameExporter.h"
#include "editor/JsonDocument.h"
#include "editor/EditorDiagnostics.h"
#include "editor/EditorContentAuthoring.h"
#include <map>
#include <tuple>
#include <framework/scene/SceneSettings.h>
#include <framework/ProjectSettings.h>
#include <framework/GameModule.h>
#include "editor/EditorExtensions.h"
#include "editor/EditorProject.h"
#include "editor/AutosaveStore.h"
#include "editor/ContentWatch.h"
#include "editor/AssetOperations.h"

namespace vega
{
	ly::Application* CreateEditorApplication(std::unique_ptr<GameModule> module);
	class EditorApplication : public ly::Application
	{
	public:
		explicit EditorApplication(std::unique_ptr<GameModule> module);
		~EditorApplication() override;
		int GetExitCode() const override;

	protected:
		bool HandleApplicationEvent(const sf::Event& event) override;
		void AfterRender() override;

	private:
		EditorApplication(std::unique_ptr<GameModule> module, EditorProject project);
		void Tick(float deltaTime) override { mModule->Tick(deltaTime); }
		void Render() override;
		void ApplySceneSettings(const nlohmann::json& settings) { mModule->ApplySceneSettings(settings); }
		EditorProject mProject;
		std::unique_ptr<GameModule> mModule;
		EditorExtensions mExtensions;
		void BuildUI();
		void BuildToolbar();
		void BuildStatusBar();
		void BuildGizmo();
		bool BuildPropertyOverlays();
		bool BuildBoundsOverlay();
		int mBoundsCorner = -1;
		nlohmann::json mBoundsStart;
		std::uint64_t mBoundsMergeGroup = 0;
		void DrawWorldOverlays(sf::RenderTexture& texture, ly::World& world);
		void BuildViewportPanel();
		void BuildHierarchyPanel();
		void BuildWorldSettingsPanel();
		void InstallWorldSettingsLayout();
		bool mShowWorldSettings = true;
		bool mSavedWorldSettingsVisible = true;
		std::optional<std::uint64_t> mSettingsPanelRevision;
		std::uint64_t mSettingsSchemaRevision = 0;
		struct SettingRow
		{
			ly::SceneSettingField field;
			nlohmann::json value;
			std::string preview, warning;
			std::vector<ly::SceneSettingOption> options;
			bool authored = false;
		};
		std::map<std::string, std::vector<SettingRow>> mSettingsRows;
		std::vector<std::pair<std::string, std::string>> mOtherSettings;
		std::uint64_t mSettingMergeGroup = 0;
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
		void AssetItemActions(const std::filesystem::path& relative, bool folder);
		void AssetDropTarget(const std::filesystem::path& folder);
		void BuildAssetDialogs();
		bool AssetsCanChange() const;
		void RefreshAfterAssetMove(const std::filesystem::path& from, const std::filesystem::path& to);
		bool ExecuteAssetCommand(const EditorScriptCommand&, std::string& detail);
		bool ExecuteAssetDelete(const AssetMovePlan& plan);
		void RequestAssetMove(const std::filesystem::path& from, const std::filesystem::path& to);
		std::unique_ptr<AssetMovePlan> mAssetMove;
		std::filesystem::path mSelectedAsset, mAssetDialogSource;
		bool mAssetPathDialog = false, mAssetRename = false, mAssetPreview = false, mAssetDelete = false;
		char mAssetPathInput[1024]{};
		std::string mAssetRecoveryError;
		void PickViewportActor();
		bool OpenScene(const std::filesystem::path& path, bool allowMissing = false, const nlohmann::json* recovered = nullptr);
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
		nlohmann::json CapturePrefab() const;
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
		std::string mPrefabRootGuid;
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
		int mProjectAutosaveSeconds = 120;
		std::string mProjectError;
		nlohmann::json mSceneSettings = nlohmann::json::object();
		std::uint64_t mSavedSceneRevision = 0;
		std::string mWindowTitle;
		nlohmann::json mPlaySnapshot;
		EditorCamera mEditCamera;
		EditorSelection mEditSelection;
		EditorSelection mRestoreSelection;
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
		bool mContentFocused = false;

		EditorViewport mViewport;
		EditorDiagnostics mDiagnostics;
		EditorContentAuthoring mContentAuthoring;
		EditorSelection mSelection;
		EditorCamera mCamera;
		JsonDocument mDocument{ mProject.assetRoot };
		AutosaveStore mAutosaves;
		std::chrono::steady_clock::time_point mNextAutosave;
		std::vector<AutosaveEntry> mRecoveryEntries;
		bool mRecoveryRequested = false;
		std::string mAutosaveError;
		ContentWatch mContentWatch;
		std::chrono::steady_clock::time_point mNextContentPoll;
		void PollContentReload();
		bool AutosaveNow();
		void PollAutosave();
		void BuildRecoveryDialog();
		bool RecoverAutosave(const AutosaveEntry& entry);
		bool ExecuteAutosaveCommand(const EditorScriptCommand&, std::string& detail);
		std::filesystem::path mLuaTypesPath;
		void RegenerateLuaTypes();
		EditorContext mContext{ nullptr, mSelection, &mCamera };
		CommandHistory mHistory{ mContext };
		std::unique_ptr<EditorScript> mScript;
			// Runtime and authoring paths come from the project file.
			GameExportLayout mExportLayout{ mProject.exportLayout };
			// The worker owns copies of its inputs and never accesses UI or world state.
			bool mExportDialogRequested = false;
			std::future<GameExportResult> mExportFuture;
			char mExportDirectory[512]{};
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
		MultiInspectorCache mMultiInspector;
		std::optional<std::tuple<unsigned int, std::uint64_t, std::uint64_t>> mInspectorCacheRevision;
		std::map<std::pair<void*, const ly::reflect::PropertyInfo*>, ly::reflect::PropertyValue> mInspectorValues;
		struct ReferencePick
		{
			unsigned int actor;
			std::optional<ly::ComponentIdentity> component;
			std::string property;
			int index = -1;
		};
		std::optional<ReferencePick> mReferencePick;
		struct AuthoringProperty
		{
			unsigned int actor;
			std::optional<ly::ComponentIdentity> component;
			const ly::reflect::PropertyInfo* property;
			ly::reflect::PropertyValue value;
		};
		std::vector<AuthoringProperty> mAuthoringProperties;
		std::optional<std::uint64_t> mAuthoringRevision, mAuthoringSelection;
		unsigned int mAuthoringWorld = 0;
		std::optional<std::pair<std::size_t, std::size_t>> mPointDrag;
		std::uint64_t mPointMergeGroup = 0;
		std::optional<unsigned int> mHierarchyAnchor;
		std::vector<SelectionTransform> mGizmoStarts;
		bool mMarquee = false, mMarqueeAdd = false, mMarqueeRemove = false;
		sf::Vector2f mMarqueeStart{}, mMarqueeScreen{};
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
