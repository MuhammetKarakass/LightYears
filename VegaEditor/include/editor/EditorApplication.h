#pragma once

#include "editor/EditorViewport.h"
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
		void BuildStatsPanel();

		EditorViewport mViewport;
		sf::Clock mUIClock;
		bool mShowViewport = true;
		bool mShowStats = true;
	};
}
