#include "widget/HUD.h"
#include "widget/Widget.h"

namespace ly
{
	namespace
	{
		// World::Render draws the HUD with the window's default view, so anchors must
		// resolve in that coordinate space rather than in raw window pixels.
		sf::Vector2u HUDViewSize(const sf::RenderWindow& window)
		{
			const sf::Vector2f size = window.getDefaultView().getSize();
			return { static_cast<unsigned int>(size.x), static_cast<unsigned int>(size.y) };
		}
	}

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
		for (auto& layer : mLayers)
		{
			layer = std::make_shared<Panel>();
			layer->SetLayout(UILayout::Stretch());
		}
	}
	void HUD::NativeInit(sf::RenderWindow& windowRef)
	{
		if (!mHasInit)
		{
			mWindowRef = &windowRef;
			Init(windowRef);
			SetViewportSize(HUDViewSize(windowRef));
			mHasInit = true;
		}
	}

	bool HUD::HandleEvent(const sf::Event& event)
	{
		if (event.is<sf::Event::Resized>() && mWindowRef) SetViewportSize(HUDViewSize(*mWindowRef));

		const bool modalCapturesEvent = HasVisibleModalChild();
		const auto tooltip = static_cast<std::size_t>(UILayer::Tooltip);
		const auto modal = static_cast<std::size_t>(UILayer::Modal);
		if (mLayers[tooltip]->NativeHandleEvent(event)) return true;
		if (mLayers[modal]->NativeHandleEvent(event)) return true;
		if (modalCapturesEvent) return true;
		for (std::size_t i = modal; i > 0; --i)
			if (mLayers[i - 1]->NativeHandleEvent(event)) return true;

		for (std::size_t i = mWidgets.size(); i > 0; --i)
		{
			if (i > mWidgets.size()) continue;
			const shared_ptr<Widget> widget = mWidgets[i - 1];
			if (widget && !widget->IsExpired() && widget->NativeHandleEvent(event)) return true;
		}
		return false;
	}

	weak_ptr<Panel> HUD::GetLayer(UILayer layer)
	{
		const auto index = static_cast<std::size_t>(layer);
		return index < LayerCount ? mLayers[index] : weak_ptr<Panel>{};
	}

	void HUD::SetViewportSize(const sf::Vector2u& size)
	{
		mViewportSize = size;
		mViewportDirty = true;
	}

	bool HUD::HasVisibleModalChild() const
	{
		const auto& modal = mLayers[static_cast<std::size_t>(UILayer::Modal)];
		if (!modal || !modal->GetVisibility() || modal->IsExpired()) return false;
		for (const auto& child : modal->GetChildren())
			if (child && child->GetVisibility() && !child->IsExpired()) return true;
		return false;
	}

	void HUD::Tick(float deltaTime)
	{
		if (mWindowRef && HUDViewSize(*mWindowRef) != mViewportSize) SetViewportSize(HUDViewSize(*mWindowRef));
		const bool viewportChanged = mViewportDirty;
		mViewportDirty = false;

		for(auto& widget : mWidgets)
		{
			widget->NativeTick(deltaTime);
		}

		auto it = std::remove_if(mWidgets.begin(), mWidgets.end(),
			[](const shared_ptr<Widget>& widget)
			{
				return widget->IsExpired();
			});
		if(it != mWidgets.end())
		{
			mWidgets.erase(it, mWidgets.end());
		}

		const UIRect viewport{ { 0.f, 0.f }, { static_cast<float>(mViewportSize.x), static_cast<float>(mViewportSize.y) } };
		for (const auto& layer : mLayers) layer->NativeTick(deltaTime);
		for (const auto& layer : mLayers) layer->ResolveLayoutTree(viewport, viewportChanged);
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
		for (auto& widget : mWidgets)
		{
			widget->NativeDraw(windowRef);
		}
		for (const auto& layer : mLayers) layer->NativeDraw(windowRef);
	}
}
