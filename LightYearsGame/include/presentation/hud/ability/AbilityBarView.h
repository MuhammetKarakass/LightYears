#pragma once

#include "presentation/hud/ability/AbilityBarViewModel.h"
#include "widget/ImageWidget.h"
#include "widget/Panel.h"
#include "widget/StackPanel.h"
#include "widget/TextWidget.h"

namespace vega
{
	class AbilityBarView : public Panel
	{
	public:
		explicit AbilityBarView(EngineContext& context, shared_ptr<const AbilityBarViewModel> viewModel);
		void Tick(float deltaTime) override;
		unsigned int GetRefreshCount() const { return mRefreshCount; }
		shared_ptr<Panel> GetColumn(std::size_t index) const { return mColumns[index].lock(); }
		shared_ptr<ImageWidget> GetIcon(std::size_t index) const { return mIcons[index].lock(); }

	private:
		struct SlotWidgets
		{
			weak_ptr<Panel> column;
			weak_ptr<TextWidget> stats;
			weak_ptr<TextWidget> input;
			weak_ptr<ImageWidget> icon;
			weak_ptr<TextWidget> state;
		};

		shared_ptr<const AbilityBarViewModel> mViewModel;
		UIRevisionWatcher mWatcher;
		weak_ptr<TextWidget> mWeaponReadout;
		std::array<weak_ptr<Panel>, 4> mGroups;
		std::array<weak_ptr<Panel>, 8> mColumns;
		std::array<weak_ptr<ImageWidget>, 8> mIcons;
		std::array<SlotWidgets, 8> mSlots;
		std::uint32_t mIconGeneration{ 0 };
		std::array<std::string, 8> mIconPaths;
		unsigned int mRefreshCount{ 0 };
	};
}
