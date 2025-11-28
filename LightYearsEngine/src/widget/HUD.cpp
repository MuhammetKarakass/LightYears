#include "widget/HUD.h"

namespace ly
{
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
		// Default implementation does nothing
	}

	void HUD::Init(sf::RenderWindow& windowRef)
	{
		// Default implementation does nothing
	}
}