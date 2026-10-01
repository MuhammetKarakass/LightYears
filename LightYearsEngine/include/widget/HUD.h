#pragma once

#include <SFML/Graphics.hpp>
#include <array>
#include <cstdint>
#include "framework/Object.h"
#include "widget/Panel.h"
#include "widget/UILayout.h"


namespace ly

{
	class Widget;
	enum class UILayer : std::uint8_t { Hud = 0, Menu, Modal, Tooltip, Count };

	class HUD : public Object
	{
	public:
		static constexpr std::size_t LayerCount = static_cast<std::size_t>(UILayer::Count);

		virtual void Draw(sf::RenderWindow& windowRef);
		void NativeInit(sf::RenderWindow& windowRef);

		const bool HasInit() const { return mHasInit; }
		virtual bool HandleEvent(const sf::Event& event);

		virtual void Tick(float deltaTime);

		template<typename T, typename ...Args>
		weak_ptr<T> AddWidget(Args&&... args)
		{
			shared_ptr<T> newWidget{ new T(args...) };
			mWidgets.push_back(newWidget);
			return newWidget;
		};

		void RemoveWidgetByTag(const std::string& tagToRemove);
		void RemoveWidget(const weak_ptr<Widget>& widgetToRemove);

		weak_ptr<Panel> GetLayer(UILayer layer);
		template<typename T, typename... Args>
		weak_ptr<T> AddToLayer(UILayer layer, Args&&... args)
		{
			if (auto panel = GetLayer(layer).lock()) return panel->AddChild<T>(std::forward<Args>(args)...);
			return {};
		}
		void SetViewportSize(const sf::Vector2u& size);
		sf::Vector2u GetViewportSize() const { return mViewportSize; }

	protected:

		HUD();
		List<shared_ptr<Widget>> mWidgets;

	private:
		bool mHasInit;
		sf::RenderWindow* mWindowRef{ nullptr };
		sf::Vector2u mViewportSize{};
		bool mViewportDirty{ true };
		std::array<shared_ptr<Panel>, LayerCount> mLayers;

		virtual void Init(sf::RenderWindow& windowRef);
		bool HasVisibleModalChild() const;
	};
}
