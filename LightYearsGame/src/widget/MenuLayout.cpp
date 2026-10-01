#include "widget/MenuLayout.h"

namespace ly
{
	weak_ptr<StackPanel> BuildMenuColumn(Panel& parent, const UILayout& layout, float spacing)
	{
		auto column = parent.AddChild<StackPanel>(UIOrientation::Vertical, spacing);
		if (auto locked = column.lock())
		{
			locked->SetCrossAlign(UIAlign::Center);
			locked->SetLayout(layout);
		}
		return column;
	}

	weak_ptr<Button> AddMenuButton(StackPanel& column, const std::string& text, unsigned int textSize)
	{
		auto button = column.AddChild<Button>(text);
		if (auto locked = button.lock()) locked->SetTextSize(textSize);
		return button;
	}
}
