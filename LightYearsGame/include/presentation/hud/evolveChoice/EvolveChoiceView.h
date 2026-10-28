#pragma once

#include "presentation/hud/evolveChoice/EvolveChoiceViewModel.h"
#include "widget/Panel.h"
#include "widget/TextWidget.h"
#include <array>

namespace vega
{
	class EvolveChoiceView : public Panel
	{
	public:
		EvolveChoiceView(EngineContext& context, std::shared_ptr<const EvolveChoiceViewModel> viewModel);
		void Tick(float deltaTime) override;
	private:
		std::shared_ptr<const EvolveChoiceViewModel> mViewModel;
		UIRevisionWatcher mWatcher;
		weak_ptr<TextWidget> mTitle;
		std::array<weak_ptr<TextWidget>, 3> mOptions;
	};
}
