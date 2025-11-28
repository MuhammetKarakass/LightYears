#pragma once

#include <SFML/Graphics.hpp>
#include "framework/Object.h"

namespace ly
{
	class HUD : public Object
	{
	public:
		
		virtual void Draw(sf::RenderWindow& windowRef) = 0;
		void NativeInit(sf::RenderWindow& windowRef);

		const bool HasInit() const { return mHasInit; }
		virtual bool HandleEvent(const sf::Event& event);

		virtual void Tick(float deltaTime);

	protected:

		HUD();

	private:
		bool mHasInit;

		virtual void Init(sf::RenderWindow& windowRef);
	};
}