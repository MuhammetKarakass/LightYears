#include "widget/HUD.h"
#include "widget/Widget.h"

namespace ly
{
	void HUD::RemoveWidgetByTag(const std::string& tagToRemove)
	{
		for (auto& widget : mWidgets)
		{
			if(widget->GetTag() == tagToRemove)
			{
				widget->DestroyWidget();
			}
		}
	}
	void HUD::RemoveWidget(const weak_ptr<Widget>& widgetToRemove)
	{
		auto it = std::remove(mWidgets.begin(), mWidgets.end(), widgetToRemove.lock());
		mWidgets.erase(it, mWidgets.end());
	}
	HUD::HUD():
		mHasInit{false}
	{
	}
	void HUD::NativeInit(sf::RenderWindow& windowRef)
	{
		if (!mHasInit)
		{
			Init(windowRef);
			mHasInit = true;
		}
	}

	bool HUD::HandleEvent(const sf::Event& event)
	{
		return false;
	}

	void HUD::Tick(float deltaTime)
	{
		/*for(auto& widget : mWidgets)
		{
			widget->NativeTick(deltaTime);
		}*/

		auto it = std::remove_if(mWidgets.begin(), mWidgets.end(),
			[](const shared_ptr<Widget>& widget)
			{
				return widget->IsExpired();
			});
		if(it != mWidgets.end())
		{
			mWidgets.erase(it, mWidgets.end());
		}
	}

	void HUD::Init(sf::RenderWindow& windowRef)
	{
		/*for(auto& widget : mWidgets)
		{
			widget->SetVisibility(true);
		}*/
	}

	void HUD::Draw(sf::RenderWindow& windowRef)
	{
		/*for (auto& widget : mWidgets)
		{
			widget->NativeDraw(windowRef);
		}*/
	}
}