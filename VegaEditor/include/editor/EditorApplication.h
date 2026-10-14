#pragma once

#include "editor/EditorViewport.h"
#include "editor/EditorSelection.h"
#include <gameFramework/GameApplication.h>

namespace vega
{
	class EditorApplication : public ly::GameApplication
	{
	public:
		EditorApplication();
		~EditorApplication() override;

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
		sf::Clock mUIClock;
		sf::FloatRect mViewportImageRect{};
		char mHierarchyFilter[128]{};
		bool mViewportImageHovered = false;
		bool mConsumeSelectionClick = false;
		bool mSelectionClickPending = false;
		bool mDockLayoutInitialized = false;
		bool mShowViewport = true;
		bool mShowHierarchy = true;
		bool mShowInspector = true;
		bool mShowStats = true;
	};
}
