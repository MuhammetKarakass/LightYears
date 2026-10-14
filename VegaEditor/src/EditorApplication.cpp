#include "editor/EditorApplication.h"

#include <framework/World.h>
#include <framework/Actor.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui-SFML.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <typeinfo>

#ifdef __GNUG__
#include <cxxabi.h>
#endif

namespace
{
	std::string ActorTypeName(const ly::Actor& actor)
	{
		std::string name = typeid(actor).name();
#ifdef __GNUG__
		int status = 0;
		std::unique_ptr<char, decltype(&std::free)> demangled{
			abi::__cxa_demangle(name.c_str(), nullptr, nullptr, &status), &std::free };
		if (status == 0 && demangled)
		{
			name = demangled.get();
		}
#endif
		// MSVC RTTI is already demangled, but includes the class/struct keyword.
		if (name.compare(0, 6, "class ") == 0)
		{
			name.erase(0, 6);
		}
		if (name.compare(0, 7, "struct ") == 0)
		{
			name.erase(0, 7);
		}
		const auto scope = name.rfind("::");
		if (scope != std::string::npos)
		{
			name.erase(0, scope + 2);
		}
		return name;
	}
}

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
		// Image clicks select actors; panel dragging belongs on the title/tab bars.
		ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;
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
			if (const auto actor = mSelection.Get())
			{
				if (actor->GetWorld() == world)
				{
					const sf::View previousView = texture.getView();
					const sf::View worldView = world->GetWorldView();
					const sf::FloatRect bounds = actor->GetActorGlobalBounds();
					sf::RectangleShape outline{ bounds.size };
					outline.setPosition(bounds.position);
					outline.setFillColor(sf::Color::Transparent);
					outline.setOutlineColor(sf::Color{ 74, 210, 255 });
					// Compensate for camera zoom to keep the outline two texture pixels wide.
					const float viewPixels = static_cast<float>(texture.getSize().x) * worldView.getViewport().size.x;
					outline.setOutlineThickness(viewPixels > 0.f ? 2.f * worldView.getSize().x / viewPixels : 2.f);
					texture.setView(worldView);
					texture.draw(outline);
					texture.setView(previousView);
				}
				else
				{
					// A world swap must not leave an externally retained actor selected.
					mSelection.Clear();
				}
			}
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
		bool resetLayout = false;
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
				ImGui::MenuItem("Hierarchy", nullptr, &mShowHierarchy);
				ImGui::MenuItem("Inspector", nullptr, &mShowInspector);
				ImGui::MenuItem("Stats", nullptr, &mShowStats);
				ImGui::Separator();
				resetLayout = ImGui::MenuItem("Reset Layout");
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}
		const ImGuiID dockspace = ImHashStr("VegaDockSpace");
		const auto* viewport = static_cast<ImGuiViewportP*>(ImGui::GetMainViewport());
		// WorkPos includes menu reservations from the previous frame. The build work
		// rect includes this frame's menu too, preventing overlap even on first launch.
		const ImRect workRect = viewport->GetBuildWorkRect();
		if (resetLayout || (!mDockLayoutInitialized && !ImGui::DockBuilderGetNode(dockspace)))
		{
			ImGui::DockBuilderRemoveNode(dockspace);
			ImGui::DockBuilderAddNode(dockspace, ImGuiDockNodeFlags_DockSpace);
			ImGui::DockBuilderSetNodePos(dockspace, workRect.Min);
			ImGui::DockBuilderSetNodeSize(dockspace, workRect.GetSize());
			ImGuiID centre = dockspace;
			const ImGuiID left = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Left, 0.18f, nullptr, &centre);
			// The second split uses the remaining width, so the right column is 24% overall.
			ImGuiID right = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Right, 0.24f / 0.82f, nullptr, &centre);
			const ImGuiID stats = ImGui::DockBuilderSplitNode(right, ImGuiDir_Down, 0.25f, nullptr, &right);
			ImGui::DockBuilderDockWindow("Viewport", centre);
			ImGui::DockBuilderDockWindow("Hierarchy", left);
			ImGui::DockBuilderDockWindow("Inspector", right);
			ImGui::DockBuilderDockWindow("Stats", stats);
			ImGui::DockBuilderFinish(dockspace);
			mShowViewport = mShowHierarchy = mShowInspector = mShowStats = true;
		}
		mDockLayoutInitialized = true;
		ImGui::SetNextWindowPos(workRect.Min);
		ImGui::SetNextWindowSize(workRect.GetSize());
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 0.f });
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
		ImGui::Begin("##VegaDockHost", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoSavedSettings);
		ImGui::PopStyleVar(3);
		ImGui::DockSpace(dockspace);
		ImGui::End();

		// Hidden and collapsed panels must not retain last frame's input focus.
		mViewport.SetImagePresentation({}, false, false);
		mViewportImageRect = {};
		mViewportImageHovered = false;
		if (const auto actor = mSelection.Get(); actor && actor->GetWorld() != GetCurrentWorld())
		{
			mSelection.Clear();
		}
		if (mShowViewport)
		{
			BuildViewportPanel();
		}
		mSelectionClickPending = false;
		if (mShowHierarchy)
		{
			BuildHierarchyPanel();
		}
		if (mShowInspector)
		{
			BuildInspectorPanel();
		}
		if (mShowStats)
		{
			BuildStatsPanel();
		}
	}

	void EditorApplication::BuildViewportPanel()
	{
		const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
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
			ImGui::SetItemTooltip("Paused: click an actor in the viewport to select it; the editor consumes the click.\n"
				"Playing: clicks go to the game; select actors in the Hierarchy.");
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
				mViewportImageRect = { { topLeft.x - mainViewport->Pos.x, topLeft.y - mainViewport->Pos.y },
					{ bottomRight.x - topLeft.x, bottomRight.y - topLeft.y } };
				mViewportImageHovered = ImGui::IsItemHovered();
				mViewport.SetImagePresentation(mViewportImageRect, mViewportImageHovered, ImGui::IsWindowFocused());
				if (world && world->IsPaused() && mSelectionClickPending && mViewportImageHovered &&
					!ImGui::IsAnyItemActive() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				{
					PickViewportActor();
				}
			}
		}
		ImGui::End();
	}

	void EditorApplication::BuildHierarchyPanel()
	{
		if (ImGui::Begin("Hierarchy", &mShowHierarchy))
		{
			ImGui::InputTextWithHint("##ActorFilter", "Filter by type name", mHierarchyFilter, sizeof(mHierarchyFilter));
			ImGui::Separator();
			struct Row
			{
				std::shared_ptr<ly::Actor> actor;
				std::string name;
			};
			std::vector<Row> rows;
			if (const ly::World* world = GetCurrentWorld())
			{
				const std::string filter = mHierarchyFilter;
				for (const auto& weakActor : world->GetActorsByType<ly::Actor>())
				{
					if (auto actor = weakActor.lock(); actor && !actor->GetIsPendingDestroy())
					{
						std::string name = ActorTypeName(*actor);
						if (filter.empty() || std::search(name.begin(), name.end(), filter.begin(), filter.end(),
							[](unsigned char first, unsigned char second) { return std::tolower(first) == std::tolower(second); }) != name.end())
						{
							rows.push_back({ std::move(actor), std::move(name) });
						}
					}
				}
			}
			const auto selected = mSelection.Get();
			// Filter before clipping: every displayed index must correspond to one visible row.
			if (ImGui::BeginChild("ActorList"))
			{
				ImGuiListClipper clipper;
				clipper.Begin(static_cast<int>(rows.size()));
				while (clipper.Step())
				{
					for (int index = clipper.DisplayStart; index < clipper.DisplayEnd; ++index)
					{
						const Row& row = rows[static_cast<std::size_t>(index)];
						const std::string label = row.name + "  #" + std::to_string(row.actor->GetUniqueID());
						ImGui::PushID(row.actor.get());
						if (ImGui::Selectable(label.c_str(), row.actor == selected))
						{
							mSelection.Select(row.actor);
						}
						ImGui::PopID();
					}
				}
			}
			ImGui::EndChild();
		}
		ImGui::End();
	}

	void EditorApplication::BuildInspectorPanel()
	{
		if (ImGui::Begin("Inspector", &mShowInspector))
		{
			if (const auto actor = mSelection.Get())
			{
				ImGui::TextWrapped("%s  #%u", ActorTypeName(*actor).c_str(), actor->GetUniqueID());
				if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
				{
					const sf::Vector2f location = actor->GetActorLocation();
					float values[]{ location.x, location.y };
					if (ImGui::DragFloat2("Location", values, 1.f))
					{
						actor->SetActorLocation({ values[0], values[1] });
					}
					float rotation = actor->GetActorRotation();
					if (ImGui::DragFloat("Rotation", &rotation, 0.5f))
					{
						actor->SetActorRotation(rotation);
					}
					const sf::Vector2f velocity = actor->GetVelocity();
					ImGui::Text("Velocity: %.2f, %.2f", velocity.x, velocity.y);
				}
				if (ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen))
				{
					static const char* layers[]{ "Background", "GroundDecal", "World", "Projectile", "WorldVfx", "Foreground" };
					static_assert(IM_ARRAYSIZE(layers) == static_cast<int>(ly::RenderLayer::Count), "Update editor render layers");
					int layer = static_cast<int>(actor->GetRenderLayer());
					if (ImGui::Combo("RenderLayer", &layer, layers, IM_ARRAYSIZE(layers)))
					{
						actor->SetRenderLayer(static_cast<ly::RenderLayer>(layer));
					}
					bool enabled = actor->IsRenderEnabled();
					if (ImGui::Checkbox("Render enabled", &enabled))
					{
						actor->SetRenderEnabled(enabled);
					}
				}
				if (ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::Text("Physics enabled: %s", actor->IsPhysicsEnabled() ? "Yes" : "No");
					ImGui::Text("Has body: %s", actor->HasPhysicsBody() ? "Yes" : "No");
					ImGui::Text("Body type: %s", actor->GetPhysicsBodyType() == ly::PhysicsBodyType::Static ? "Static" : "Dynamic");
					ImGui::Text("Collision layer: 0x%02X", static_cast<unsigned int>(actor->GetCollisionLayer()));
					ImGui::Text("Collision mask: 0x%02X", static_cast<unsigned int>(actor->GetCollisionMask()));
				}
				if (ImGui::CollapsingHeader("Bounds", ImGuiTreeNodeFlags_DefaultOpen))
				{
					const sf::FloatRect bounds = actor->GetActorGlobalBounds();
					ImGui::Text("Position: %.2f, %.2f", bounds.position.x, bounds.position.y);
					ImGui::Text("Size: %.2f, %.2f", bounds.size.x, bounds.size.y);
				}
				ImGui::Separator();
				if (ImGui::Button("Destroy"))
				{
					actor->Destroy();
					mSelection.Clear();
					ImGui::TextUnformatted("Pending destroy");
				}
			}
			else
			{
				ImGui::TextWrapped("Select an actor in the Hierarchy or paused viewport.");
			}
		}
		ImGui::End();
	}

	void EditorApplication::PickViewportActor()
	{
		ly::World* world = GetCurrentWorld();
		if (!world)
		{
			return;
		}
		const sf::Vector2f point = world->GetMouseWorldPosition();
		const auto actors = world->GetActorsInBounds({ point - sf::Vector2f{ 2.f, 2.f }, { 4.f, 4.f } });
		std::vector<ActorPickBounds> candidates;
		for (const auto& weakActor : actors)
		{
			if (const auto actor = weakActor.lock(); actor && !actor->GetIsPendingDestroy())
			{
				candidates.emplace_back(actor->GetUniqueID(), actor->GetActorGlobalBounds());
			}
		}
		const auto best = PickBestActor(point, candidates);
		mSelection.Clear();
		if (best)
		{
			for (const auto& weakActor : actors)
			{
				if (const auto actor = weakActor.lock(); actor && actor->GetUniqueID() == *best)
				{
					mSelection.Select(actor);
					break;
				}
			}
		}
	}

	void EditorApplication::BuildStatsPanel()
	{
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
		sf::Vector2i mousePixel = sf::Mouse::getPosition(GetRenderWindow());
		if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>())
		{
			mousePixel = pressed->position;
		}
		if (const auto* released = event.getIf<sf::Event::MouseButtonReleased>())
		{
			mousePixel = released->position;
		}
		if (const auto* moved = event.getIf<sf::Event::MouseMoved>())
		{
			mousePixel = moved->position;
		}
		if (const auto* wheel = event.getIf<sf::Event::MouseWheelScrolled>())
		{
			mousePixel = wheel->position;
		}
		const sf::Vector2f mousePosition{ mousePixel };
		const ImVec2 viewportPosition = ImGui::GetMainViewport()->Pos;
		ImGuiWindow* hoveredWindow = nullptr;
		// Events arrive before NewFrame. Re-hit-test their position so a click that
		// just crossed into an Inspector control or a floating panel cannot reach gameplay.
		ImGui::FindHoveredWindowEx({ mousePosition.x + viewportPosition.x, mousePosition.y + viewportPosition.y },
			true, &hoveredWindow, nullptr);
		const bool overImage = mViewportImageRect.contains(mousePosition) && hoveredWindow &&
			hoveredWindow == ImGui::FindWindowByName("Viewport") && !ImGui::IsAnyItemActive() &&
			!ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
		// Selection owns viewport left clicks only while paused. While playing,
		// gameplay receives them and editor selection is available via Hierarchy.
		// Remember the press so its release is consumed even if the cursor leaves the image.
		if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>())
		{
			const ly::World* world = GetCurrentWorld();
			if (pressed->button == sf::Mouse::Button::Left && world && world->IsPaused() &&
				overImage)
			{
				mConsumeSelectionClick = true;
				mSelectionClickPending = true;
				return true;
			}
		}
		if (const auto* released = event.getIf<sf::Event::MouseButtonReleased>();
			released && released->button == sf::Mouse::Button::Left && mConsumeSelectionClick)
		{
			mConsumeSelectionClick = false;
			return true;
		}
		const bool mouseEvent = event.is<sf::Event::MouseButtonPressed>() || event.is<sf::Event::MouseButtonReleased>() ||
			event.is<sf::Event::MouseMoved>() || event.is<sf::Event::MouseMovedRaw>() ||
			event.is<sf::Event::MouseWheelScrolled>() || event.is<sf::Event::MouseEntered>() || event.is<sf::Event::MouseLeft>();
		const bool keyboardEvent = event.is<sf::Event::KeyPressed>() || event.is<sf::Event::KeyReleased>();
		const bool textEvent = event.is<sf::Event::TextEntered>();
		if ((mouseEvent && io.WantCaptureMouse && !overImage) ||
			(!mViewport.HasPanelInputFocus() && ((keyboardEvent && io.WantCaptureKeyboard) ||
			(textEvent && (io.WantCaptureKeyboard || io.WantTextInput)))))
		{
			return true;
		}
		// Known limitation: HUD widgets still receive raw window-pixel event coordinates,
		// so clicking their buttons inside a scaled editor viewport may miss.
		return GameApplication::HandleApplicationEvent(event);
	}
}
