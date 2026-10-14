#include "framework/RenderViewport.h"
#include "framework/World.h"
#include <cmath>
#include <iostream>

namespace
{
	// RenderTexture acquires an OpenGL context even before drawing, so use an inert target for headless tests.
	class FakeRenderTarget final : public sf::RenderTarget
	{
	public:
		sf::Vector2u getSize() const override
		{
			return {};
		}
	};

	class FakeRenderViewport final : public ly::RenderViewport
	{
	public:
		sf::RenderTarget& GetRenderTarget() override
		{
			return mTarget;
		}

		const sf::RenderTarget& GetRenderTarget() const override
		{
			return mTarget;
		}

		sf::Vector2u GetSize() const override
		{
			return size;
		}

		sf::Vector2i GetMousePixelPosition() const override
		{
			return mouse;
		}

		sf::Vector2f MapPixelToCoords(sf::Vector2i pixel, const sf::View& view) const override
		{
			++mapCallCount;
			lastMappedPixel = pixel;
			lastMappedView = view;
			return view.getCenter() + sf::Vector2f{
				(static_cast<float>(pixel.x) / static_cast<float>(size.x) - 0.5f) * view.getSize().x,
				(static_cast<float>(pixel.y) / static_cast<float>(size.y) - 0.5f) * view.getSize().y
			};
		}

		sf::View GetDefaultView() const override
		{
			return defaultView;
		}

		bool HasInputFocus() const override
		{
			return focused;
		}

		sf::Vector2u size{ 800u, 600u };
		sf::Vector2i mouse{ 200, 450 };
		bool focused = true;
		sf::View defaultView{ sf::Vector2f{ 100.f, -75.f }, sf::Vector2f{ 1600.f, 900.f } };
		mutable int mapCallCount = 0;
		mutable sf::Vector2i lastMappedPixel{};
		mutable sf::View lastMappedView{};

	private:
		FakeRenderTarget mTarget;
	};

	bool Near(sf::Vector2f first, sf::Vector2f second)
	{
		return std::abs(first.x - second.x) < 1e-4f && std::abs(first.y - second.y) < 1e-4f;
	}

	int Fail(const char* message)
	{
		std::cerr << message << '\n';
		return 1;
	}
}

int main()
{
	FakeRenderViewport viewport;
	ly::World world{ nullptr };
	const ly::World& constWorld = world;
	if (world.GetRenderViewport() != nullptr || constWorld.GetRenderViewport() != nullptr ||
		world.HasInputFocus() || world.GetWindowSize() != sf::Vector2u{ 0u, 0u } ||
		!Near(world.GetWorldView().getCenter(), sf::View{}.getCenter()) ||
		!Near(world.GetMouseWorldPosition(), world.GetWorldView().getCenter()))
	{
		return Fail("A world without a viewport did not preserve the headless defaults");
	}

	world.SetRenderViewport(&viewport);
	if (world.GetRenderViewport() != &viewport || constWorld.GetRenderViewport() != &viewport ||
		world.GetWindowSize() != viewport.size || !world.HasInputFocus())
	{
		return Fail("The world did not use its viewport override for size and focus");
	}
	viewport.focused = false;
	if (world.HasInputFocus())
	{
		return Fail("The world ignored the viewport losing input focus");
	}
	viewport.focused = true;
	viewport.size = { 1000u, 750u };
	if (!world.HasInputFocus() || world.GetWindowSize() != viewport.size)
	{
		return Fail("The world did not read updated viewport size and focus");
	}
	viewport.size = { 800u, 600u };

	const sf::View worldView = constWorld.GetWorldView();
	if (!Near(worldView.getCenter(), viewport.defaultView.getCenter()) ||
		!Near(worldView.getSize(), viewport.defaultView.getSize()))
	{
		return Fail("The world view did not derive from the viewport default view");
	}
	if (!Near(constWorld.GetMouseWorldPosition(), { -300.f, 150.f }) ||
		viewport.mapCallCount != 1 || viewport.lastMappedPixel != viewport.mouse ||
		!Near(viewport.lastMappedView.getCenter(), worldView.getCenter()) ||
		!Near(viewport.lastMappedView.getSize(), worldView.getSize()))
	{
		return Fail("Mouse world position did not map the viewport mouse through the world view");
	}
	viewport.mouse = { -80, 660 };
	if (!Near(world.GetMouseWorldPosition(), { -860.f, 465.f }) ||
		viewport.mapCallCount != 2 || viewport.lastMappedPixel != viewport.mouse)
	{
		return Fail("Mouse pixels outside the viewport were clamped or mapped incorrectly");
	}

	world.SetRenderViewport(nullptr);
	if (world.GetRenderViewport() != nullptr || constWorld.GetRenderViewport() != nullptr ||
		world.HasInputFocus() || world.GetWindowSize() != sf::Vector2u{} ||
		!Near(world.GetWorldView().getCenter(), sf::View{}.getCenter()) ||
		!Near(world.GetMouseWorldPosition(), world.GetWorldView().getCenter()) ||
		viewport.mapCallCount != 2)
	{
		return Fail("Clearing the viewport override did not restore the headless fallback");
	}

	{
		ly::World cameraWorld{ nullptr };
		cameraWorld.SetRenderViewport(&viewport);
		cameraWorld.TickInternal(0.f);
		const sf::Vector2f initializedCenter = viewport.defaultView.getCenter();
		viewport.defaultView.setCenter({ 900.f, 700.f });
		if (!Near(cameraWorld.GetWorldView().getCenter(), initializedCenter))
		{
			return Fail("The camera was not initialized from the viewport during the world tick");
		}
	}

	std::cout << "Render viewport tests passed\n";
	return 0;
}
