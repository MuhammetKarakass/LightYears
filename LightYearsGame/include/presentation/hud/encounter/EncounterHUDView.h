#pragma once

#include "presentation/hud/encounter/EncounterHUDPresentation.h"
#include "widget/Panel.h"
#include "widget/StackPanel.h"
#include "widget/TextWidget.h"

namespace ly
{
	class EncounterHUDView : public Panel
	{
	public:
		explicit EncounterHUDView(shared_ptr<const EncounterHUDPresentation> presentation);
		void Tick(float deltaTime) override;
		unsigned int GetRefreshCount() const { return mRefreshCount; }
		const std::string& GetPresentedTitle() const { return mPresentation->title; }
		sf::Color GetPresentedTitleColor() const { return mPresentation->titleColor; }

	private:
		shared_ptr<const EncounterHUDPresentation> mPresentation;
		UIRevisionWatcher mWatcher;
		weak_ptr<TextWidget> mTitle;
		weak_ptr<TextWidget> mDetail;
		unsigned int mRefreshCount{ 0 };
	};
}
