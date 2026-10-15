#include "editor/EditorCamera.h"
#include <cmath>
#include <iostream>

namespace
{
	int failures = 0;
	void Check(bool condition, const char* message)
	{
		if (!condition)
		{
			std::cerr << message << '\n';
			++failures;
		}
	}
	bool Near(sf::Vector2f a, sf::Vector2f b)
	{
		return std::abs(a.x - b.x) < 0.001f && std::abs(a.y - b.y) < 0.001f;
	}
	// Normalized screen coordinates avoid needing a render target or graphics context.
	sf::Vector2f Screen(const sf::View& view, sf::Vector2f point)
	{
		const auto offset = point - view.getCenter();
		return { offset.x / view.getSize().x, offset.y / view.getSize().y };
	}
}

int main()
{
	vega::EditorCamera camera;
	camera.SetTextureSize({ 800u, 600u });
	const sf::View initial{ { 3000.f, 1500.f }, { 1600.f, 1200.f } };
	camera.InitFrom(initial);
	Check(Near(camera.GetView().getCenter(), initial.getCenter()) &&
		Near(camera.GetView().getSize(), initial.getSize()), "InitFrom changed the view");
	const sf::Vector2f anchor{ 3250.f, 1300.f };
	const auto screen = Screen(camera.GetView(), anchor);
	for (float factor : { 0.5f, 2.f, 1000.f, 0.00001f })
	{
		camera.ZoomAt(factor, anchor);
		Check(Near(Screen(camera.GetView(), anchor), screen), "Zoom moved the cursor anchor");
		Check(camera.GetZoom() >= 0.05f && camera.GetZoom() <= 20.f, "Zoom escaped limits");
	}
	Check(camera.GetZoom() == 0.05f, "Minimum zoom clamp failed");
	camera.ZoomAt(100000.f, anchor);
	Check(camera.GetZoom() == 20.f, "Maximum zoom clamp failed");
	const auto before = camera.GetView();
	camera.ZoomAt(0.f, anchor);
	camera.ZoomAt(-2.f, anchor);
	Check(Near(camera.GetView().getCenter(), before.getCenter()) && camera.GetZoom() == 20.f, "Invalid zoom changed view");
	camera.Pan({ -500.f, 70.f });
	Check(Near(camera.GetView().getCenter(), before.getCenter() + sf::Vector2f{ -500.f, 70.f }), "Pan delta incorrect");
	camera.Focus({ { 400.f, 300.f }, { 640.f, 120.f } });
	Check(Near(camera.GetView().getCenter(), { 720.f, 360.f }) &&
		Near(camera.GetView().getSize(), { 1600.f, 1200.f }), "Wide focus did not fit to 40 percent");
	camera.Focus({ { 0.f, 0.f }, { 80.f, 480.f } });
	Check(camera.GetZoom() == 2.f, "Tall focus did not fit to 40 percent");
	camera.Focus({ { 400.f, 300.f }, { 10.f, 10.f } });
	Check(camera.GetZoom() == 1.f && Near(camera.GetView().getCenter(), { 405.f, 305.f }), "Tiny focus exceeded 1:1");
	camera.Focus({ { 0.f, 0.f }, { 0.f, 0.f } });
	Check(camera.GetZoom() == 1.f, "Empty bounds focus invalid");
	camera.SetTextureSize({ 1600u, 900u });
	Check(Near(camera.GetView().getSize(), { 1600.f, 900.f }), "Resize did not preserve world units per pixel");
	return failures ? 1 : 0;
}
