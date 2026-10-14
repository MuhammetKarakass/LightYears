#include "editor/EditorViewport.h"

#include <iostream>

namespace
{
	bool Check(sf::Vector2i windowPixel, const sf::FloatRect& imageRect, sf::Vector2u textureSize,
		sf::Vector2i expected, const char* message)
	{
		if (vega::MapWindowToTexturePixel(windowPixel, imageRect, textureSize) != expected)
		{
			std::cerr << message << '\n';
			return false;
		}
		return true;
	}
}

int main()
{
	// Exercise the actual viewport mapping without constructing a window or render texture.
	const sf::FloatRect image{ { 100.f, 80.f }, { 960.f, 540.f } };
	const sf::Vector2u texture{ 1920u, 1080u };
	if (!Check({ 100, 80 }, image, texture, { 0, 0 }, "Image origin did not map to texture origin") ||
		!Check({ 580, 350 }, image, texture, { 960, 540 }, "Image centre did not map to texture centre") ||
		!Check({ 1060, 620 }, image, texture, { 1920, 1080 }, "Image edge scaling is incorrect") ||
		!Check({ 90, 630 }, image, texture, { -20, 1100 }, "Outside pixels were clamped or scaled incorrectly") ||
		!Check({ 400, 500 }, { { 200.f, 100.f }, { 400.f, 800.f } }, { 800u, 400u }, { 400, 200 }, "Axes were not scaled independently") ||
		!Check({ 151, 102 }, { { 100.5f, 80.5f }, { 101.f, 43.f } }, { 202u, 86u }, { 101, 43 }, "Fractional image placement lost precision") ||
		!Check({ 580, 350 }, { { 100.f, 80.f }, { 1920.f, 1080.f } }, texture, { 480, 270 }, "Unscaled mapping is incorrect") ||
		!Check({ 100, 80 }, { { 100.f, 80.f }, { 0.f, 540.f } }, texture, {}, "Zero width was not handled") ||
		!Check({ 100, 80 }, { { 100.f, 80.f }, { 960.f, 0.f } }, texture, {}, "Zero height was not handled"))
	{
		return 1;
	}
	std::cout << "Editor viewport mapping tests passed\n";
	return 0;
}
