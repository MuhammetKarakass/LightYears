#pragma once

#include "widget/Button.h"
#include "widget/Panel.h"
#include "widget/StackPanel.h"

namespace ly
{
	weak_ptr<StackPanel> BuildMenuColumn(Panel& parent, const UILayout& layout, float spacing);
	weak_ptr<Button> AddMenuButton(StackPanel& column, const std::string& text, unsigned int textSize);
}
