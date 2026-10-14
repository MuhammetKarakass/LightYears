#include "editor/EditorApplication.h"

#include <framework/World.h>
#include <imgui.h>
#include <imgui-SFML.h>

#include <algorithm>
#include <stdexcept>
#include <typeinfo>

namespace vega
{
	EditorApplication::EditorApplication()
		: mViewport{ GetRenderWindow() }
	{
		if (!ImGui::SFML::Init(GetRenderWindow()))
		{
			ImGui::SFML::Shutdown();
			throw std::runtime_error("Cannot initialize the editor UI");
		}
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		GetRenderWindow().setTitle("Vega Editor");
	}

	EditorApplication::~EditorApplication()
	{
		// The base application can still own a world when editor members are destroyed.
		if (ly::World* world = GetCurrentWorld())
		{
			world->SetRenderViewport(nullptr);
		}
		ImGui::SFML::Shutdown();
	}

	void EditorApplication::Render()
	{
		mViewport.ResizeToWindow();
		sf::RenderTexture& texture = mViewport.GetTexture();
		texture.clear(sf::Color::Black);
		if (ly::World* world = GetCurrentWorld())
		{
			// LoadWorld swaps the active world, so rebind the non-owning viewport every frame.
			world->SetRenderViewport(&mViewport);
			world->Render(texture);
		}
		texture.display();
	}

	void EditorApplication::AfterRender()
	{
		sf::RenderWindow& window = GetRenderWindow();
		ImGui::SFML::Update(window, mUIClock.restart());
		BuildUI();
		ImGui::SFML::Render(window);
		// The world is drawn into the viewport texture, so the window holds the finished frame only once the UI is on
		// it (and only then is the window's framebuffer the active one); run the --visual-capture step after that.
		GameApplication::AfterRender();
	}

	void EditorApplication::BuildUI()
	{
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Exit"))
				{
					QuitApplication();
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("View"))
			{
				ImGui::MenuItem("Viewport", nullptr, &mShowViewport);
				ImGui::MenuItem("Stats", nullptr, &mShowStats);
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}
		ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

		// Hidden and collapsed panels must not retain last frame's input focus.
		mViewport.SetImagePresentation({}, false, false);
		if (mShowViewport)
		{
			BuildViewportPanel();
		}
		if (mShowStats)
		{
			BuildStatsPanel();
		}
	}

	void EditorApplication::BuildViewportPanel()
	{
		const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(mainViewport->WorkPos, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize({ mainViewport->WorkSize.x * 0.76f, mainViewport->WorkSize.y }, ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Viewport", &mShowViewport))
		{
			ly::World* world = GetCurrentWorld();
			ImGui::BeginDisabled(world == nullptr);
			const bool paused = world && world->IsPaused();
			if (ImGui::Button(paused ? "Play" : "Pause") && world)
			{
				world->SetPaused(!paused);
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			ImGui::TextUnformatted(world ? (world->IsPaused() ? "Paused" : "Playing") : "No world");
			ImGui::Separator();

			const ImVec2 available = ImGui::GetContentRegionAvail();
			if (available.x > 0.f && available.y > 0.f)
			{
				const sf::Vector2f textureSize{ mViewport.GetSize() };
				const float scale = std::min(available.x / textureSize.x, available.y / textureSize.y);
				const sf::Vector2f imageSize = textureSize * scale;
				const ImVec2 cursor = ImGui::GetCursorScreenPos();
				ImGui::SetCursorScreenPos({ cursor.x + (available.x - imageSize.x) * 0.5f, cursor.y + (available.y - imageSize.y) * 0.5f });
				// The RenderTexture overload also flips the UVs to display the image upright.
				ImGui::Image(mViewport.GetTexture(), imageSize);
				const ImVec2 topLeft = ImGui::GetItemRectMin();
				const ImVec2 bottomRight = ImGui::GetItemRectMax();
				mViewport.SetImagePresentation(
					{ { topLeft.x - mainViewport->Pos.x, topLeft.y - mainViewport->Pos.y },
					  { bottomRight.x - topLeft.x, bottomRight.y - topLeft.y } },
					ImGui::IsItemHovered(), ImGui::IsWindowFocused());
			}
		}
		ImGui::End();
	}

	void EditorApplication::BuildStatsPanel()
	{
		const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos({ mainViewport->WorkPos.x + mainViewport->WorkSize.x * 0.76f, mainViewport->WorkPos.y }, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize({ mainViewport->WorkSize.x * 0.24f, 160.f }, ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Stats", &mShowStats))
		{
			ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
			const ly::World* world = GetCurrentWorld();
			ImGui::TextWrapped("World: %s", world ? typeid(*world).name() : "None");
		}
		ImGui::End();
	}

	bool EditorApplication::HandleApplicationEvent(const sf::Event& event)
	{
		ImGui::SFML::ProcessEvent(GetRenderWindow(), event);
		const ImGuiIO& io = ImGui::GetIO();
		const bool mouseEvent = event.is<sf::Event::MouseButtonPressed>() || event.is<sf::Event::MouseButtonReleased>() ||
			event.is<sf::Event::MouseMoved>() || event.is<sf::Event::MouseMovedRaw>() ||
			event.is<sf::Event::MouseWheelScrolled>() || event.is<sf::Event::MouseEntered>() || event.is<sf::Event::MouseLeft>();
		const bool keyboardEvent = event.is<sf::Event::KeyPressed>() || event.is<sf::Event::KeyReleased>();
		const bool textEvent = event.is<sf::Event::TextEntered>();
		if (!mViewport.HasPanelInputFocus() &&
			((mouseEvent && io.WantCaptureMouse) || (keyboardEvent && io.WantCaptureKeyboard) ||
			 (textEvent && (io.WantCaptureKeyboard || io.WantTextInput))))
		{
			return true;
		}
		// Known limitation: HUD widgets still receive raw window-pixel event coordinates,
		// so clicking their buttons inside a scaled editor viewport may miss.
		return GameApplication::HandleApplicationEvent(event);
	}
}
