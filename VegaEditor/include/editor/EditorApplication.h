#pragma once

#include "editor/EditorViewport.h"
#include "editor/EditorSelection.h"
#include "editor/EditorCommands.h"
#include "editor/EditorScript.h"
#include "editor/GameExporter.h"
#include "editor/JsonDocument.h"
#include <map>
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

		EditorViewport mViewport;
		EditorSelection mSelection;
		EditorCamera mCamera;
		JsonDocument mDocument{ VEGA_SOURCE_ASSETS_DIR };
		EditorContext mContext{ nullptr, mSelection, &mCamera };
		CommandHistory mHistory{ mContext };
		std::unique_ptr<EditorScript> mScript;
			// Build-tree locations are compile definitions of this target; see VegaEditor/CMakeLists.txt.
			GameExportLayout mExportLayout{ VEGA_GAME_BUILD_DIR, VEGA_GAME_VISUAL_ASSETS_DIR, VEGA_SOURCE_ROOT, VEGA_BUILD_ROOT };
			// Export Game dialog. Export runs in two frames so "Exporting..." is on screen before the blocking copy.
			enum class ExportPhase { Idle, Starting, Running };
			bool mExportDialogRequested = false;
			ExportPhase mExportPhase = ExportPhase::Idle;
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
