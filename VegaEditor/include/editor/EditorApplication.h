#pragma once

#include "editor/EditorViewport.h"
#include "editor/EditorSelection.h"
#include "editor/EditorCommands.h"
#include "editor/EditorScript.h"
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
		void BuildViewportPanel();
		void BuildHierarchyPanel();
		void BuildInspectorPanel();
		void BuildStatsPanel();
		void PickViewportActor();

		EditorViewport mViewport;
		EditorSelection mSelection;
		EditorCamera mCamera;
		EditorContext mContext{ nullptr, mSelection, &mCamera };
		CommandHistory mHistory{ mContext };
		std::unique_ptr<EditorScript> mScript;
		std::optional<unsigned int> mWorldID;
		std::optional<unsigned int> mMoveDragActor;
		std::optional<unsigned int> mDestroyActor;
		sf::Vector2f mMoveDragMouse{};
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
	};
}
