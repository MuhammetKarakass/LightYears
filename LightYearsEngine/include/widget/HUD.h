#pragma once

#include <SFML/Graphics.hpp>
#include "framework/Object.h"


namespace ly

{
	class Widget;
	class HUD : public Object
	{
	public:
		
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

	protected:

		HUD();
		List<shared_ptr<Widget>> mWidgets;

	private:
		bool mHasInit;

		virtual void Init(sf::RenderWindow& windowRef);
	};
}