#include "editor/EditorApplication.h"
#include "editor/EditorTheme.h"

#include <framework/World.h>
#include <framework/Actor.h>
#include <framework/scene/Scene.h>
#include <framework/scene/SceneEntityComponent.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui-SFML.h>

#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <string>

namespace vega
{
	EditorApplication::EditorApplication()
		: ly::GameApplication{ true }, mViewport{ GetRenderWindow() }
	{
		if (!ImGui::SFML::Init(GetRenderWindow()))
		{
			ImGui::SFML::Shutdown();
			throw std::runtime_error("Cannot initialize the editor UI");
		}
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		ApplyEditorTheme();
		// The default dock layout is built only when the saved layout is missing, so panels added later would open
		// floating over an older layout. Versioning the file name rebuilds the default once; bump it whenever the
		// default layout or its reserved toolbar/status area changes.
		ImGui::GetIO().IniFilename = "vega_editor_layout_v3.ini";
		// Image clicks select actors; panel dragging belongs on the title/tab bars.
		ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;
		GetRenderWindow().setTitle("Vega Editor");
		std::filesystem::path startupScene{ "scenes/sandbox.scene.json" };
#ifdef _MSC_VER
		for (int index = 1; index < __argc; ++index)
		{
			if (std::string{ __argv[index] } == "--editor-script")
			{
				mScript = std::make_unique<EditorScript>(index + 1 < __argc ? __argv[index + 1] : "", mExportLayout);
				++index;
			}
			else if (std::string{ __argv[index] } == "--editor-scene" && index + 1 < __argc)
			{
				startupScene = __argv[++index];
			}
			else if (std::string{ __argv[index] } == "--asset-root" && index + 1 < __argc)
			{
				mDocument = JsonDocument{ __argv[++index] };
			}
		}
#endif
		mContext.document = &mDocument;
		mContext.sceneDirty = [this]() { return SceneDirty(); };
		mContext.sceneAction = [this](const std::string& action, const std::string& path)
		{
			if (mContext.transitioning) return false;
			if (action == "play") return SetPlayMode(true);
			if (action == "stop") return SetPlayMode(false);
			if (mContext.playMode) return false;
			if (action == "save") return SaveScene(std::filesystem::u8path(path));
			if (SceneDirty()) { mContext.sceneError = "Unsaved scene: save before replacing it"; return false; }
			if (action == "open") return OpenScene(std::filesystem::u8path(path));
			if (action == "new") { RequestScene("new"); return true; }
			return false;
		};
		mCamera.SetTextureSize(GetRenderWindow().getSize());
		mCamera.InitFrom(sf::View{ { 960.f, 540.f }, { 1920.f, 1080.f } });
		OpenScene(startupScene, true);
		RefreshContent();
	}

	EditorApplication::~EditorApplication()
	{
		// The base application can still own a world when editor members are destroyed.
		if (ly::World* world = GetCurrentWorld())
		{
			world->SetRenderViewport(nullptr);
			world->SetViewOverride(std::nullopt);
		}
		ImGui::SFML::Shutdown();
	}

	void EditorApplication::Render()
	{
		ly::World* current = GetCurrentWorld();
		if (mPauseAfterTick)
		{
			if (current) current->SetPaused(true);
			mPauseAfterTick = false;
		}
		const std::optional<unsigned int> worldID = current ? std::optional<unsigned int>{ current->GetUniqueID() } : std::nullopt;
		if (mWorldID != worldID)
		{
			// World instances change on Play/Stop; GUID commands remain valid.
			mSelection.Clear();
			mMoveDragActor.reset();
			mDestroyActor.reset();
			mWorldID = worldID;
			mContext.world = current;
			mContext.transitioning = false;
			mContext.cameraPaused = current ? std::optional<bool>{ current->IsPaused() } : std::nullopt;
			if (current)
			{
				current->SetViewOverride(current->IsEditMode() ? std::optional<sf::View>{ mCamera.GetView() } : std::nullopt);
				if (mBaselineOnLoad)
				{
					mBaselineOnLoad = false;
					mSavedScene = CaptureScene();
					auto& entities = mSavedScene["entities"];
					std::sort(entities.begin(), entities.end(), [](const auto& a, const auto& b) { return a.at("guid") < b.at("guid"); });
				}
				mSelection.Select(mContext.ResolveGuid(mRestoreSelectionGuid));
				mRestoreSelectionGuid.clear();
			}
			mCameraPanButton.reset();
		}
		mContext.world = current;
		mViewport.ResizeToWindow();
		sf::RenderTexture& texture = mViewport.GetTexture();
		texture.clear(sf::Color::Black);
		if (ly::World* world = GetCurrentWorld())
		{
			// LoadWorld swaps the active world, so rebind the non-owning viewport every frame.
			world->SetRenderViewport(&mViewport);
			mCamera.SetTextureSize(mViewport.GetSize());
			mContext.SyncCamera();
			world->Render(texture);
			DrawWorldOverlays(texture, *world);
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
					outline.setOutlineColor(sf::Color{ 51, 199, 202 });
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
		if (mScript && !mContext.transitioning && mScript->Step(mContext, mHistory, window)) QuitApplication();
		// The world is drawn into the viewport texture, so the window holds the finished frame only once the UI is on
		// it (and only then is the window's framebuffer the active one); run the --visual-capture step after that.
		GameApplication::AfterRender();
	}

	int EditorApplication::GetExitCode() const
	{
		return mScript ? mScript->ExitCode() : 0;
	}

	void EditorApplication::BuildUI()
	{
		mHistory.PruneInvalidCommands();
		const ImGuiIO& io = ImGui::GetIO();
		if (io.KeyCtrl && !io.WantTextInput && !mContext.playMode)
		{
			if (ImGui::IsKeyPressed(ImGuiKey_Z, false))
			{
				if (io.KeyShift) mHistory.Redo();
				else mHistory.Undo();
			}
			if (ImGui::IsKeyPressed(ImGuiKey_Y, false)) mHistory.Redo();
		}
		if (!mContext.playMode && io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false) && !mDataFocused) SaveScene();
		if (!mContext.playMode && !io.WantTextInput)
		{
			if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D, false)) DuplicateSceneEntity(mContext, mHistory);
			if (ImGui::IsKeyPressed(ImGuiKey_Delete, false))
				if (auto actor = mSelection.Get(); actor && actor->FindComponent<ly::SceneEntityComponent>())
					mHistory.Execute(std::make_unique<DestroyActor>(actor->GetUniqueID()));
			if (ImGui::IsKeyPressed(ImGuiKey_F2, false)) BeginRename();
		}
		const std::string sceneTitle = (mScenePath.empty() ? "Untitled" : mScenePath.filename().string()) + (SceneDirty() ? " *" : "");
		GetRenderWindow().setTitle("Vega Editor - " + sceneTitle + (mContext.playMode ? " - PLAYING" : " - EDIT"));
		bool resetLayout = false;
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				ImGui::BeginDisabled(mContext.playMode || mContext.transitioning);
				if (ImGui::MenuItem("New Scene")) RequestScene("new");
				if (ImGui::MenuItem("Open Scene...")) mOpenSceneDialog = true;
				if (ImGui::MenuItem("Save Scene", "Ctrl+S")) SaveScene();
				if (ImGui::MenuItem("Save Scene As..."))
				{
					std::snprintf(mScenePathInput, sizeof(mScenePathInput), "%s", mScenePath.empty() ? "scenes/untitled.scene.json" : mScenePath.generic_string().c_str());
					mSaveSceneDialog = true;
				}
				ImGui::EndDisabled();
				ImGui::Separator();
				if (ImGui::MenuItem("Export Game...")) mExportDialogRequested = true;
				ImGui::Separator();
				if (ImGui::MenuItem("Exit"))
				{
					RequestScene("exit");
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Edit"))
			{
				const std::string undoLabel = mHistory.CanUndo() ? "Undo " + mHistory.UndoName() : "Undo";
				const std::string redoLabel = mHistory.CanRedo() ? "Redo " + mHistory.RedoName() : "Redo";
				if (ImGui::MenuItem(undoLabel.c_str(), "Ctrl+Z", false, !mContext.playMode && mHistory.CanUndo())) mHistory.Undo();
				if (ImGui::MenuItem(redoLabel.c_str(), "Ctrl+Y / Ctrl+Shift+Z", false, !mContext.playMode && mHistory.CanRedo())) mHistory.Redo();
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("View"))
			{
				ImGui::MenuItem("Viewport", nullptr, &mShowViewport);
				ImGui::MenuItem("Hierarchy", nullptr, &mShowHierarchy);
				ImGui::MenuItem("Inspector", nullptr, &mShowInspector);
				ImGui::MenuItem("Stats", nullptr, &mShowStats);
				ImGui::MenuItem("Content", nullptr, &mShowContent);
				ImGui::Separator();
				ImGui::MenuItem("Grid overlay", nullptr, &mContext.viewSettings.grid);
				ImGui::MenuItem("Collider overlay", nullptr, &mContext.viewSettings.colliders);
				if (ImGui::MenuItem("Data Editor", nullptr, mShowData))
				{
					if (mShowData && mDocument.Dirty()) { mPendingClose = true; mConfirmDocument = true; }
					else mShowData = !mShowData;
				}
				ImGui::Separator();
				resetLayout = ImGui::MenuItem("Reset Layout");
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}
		BuildToolbar();
		BuildStatusBar();
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
			const ImGuiID content = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Down, 0.28f, nullptr, &centre);
			ImGui::DockBuilderDockWindow("Viewport", centre);
			ImGui::DockBuilderDockWindow("Hierarchy", left);
			ImGui::DockBuilderDockWindow("Inspector", right);
			ImGui::DockBuilderDockWindow("Stats", stats);
			ImGui::DockBuilderDockWindow("Content", content);
			ImGui::DockBuilderDockWindow("Data Editor###DataEditor", content);
			ImGui::DockBuilderFinish(dockspace);
			mShowViewport = mShowHierarchy = mShowInspector = mShowStats = true;
			mShowContent = true;
			mShowData = mDocument.IsOpen();
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
		mViewportWindowFocused = false;
		if (const auto actor = mSelection.Get(); actor && actor->GetWorld() != GetCurrentWorld())
		{
			mSelection.Clear();
		}
		if (mShowViewport)
		{
			BuildViewportPanel();
		}
		if (mViewportImageRect.size == sf::Vector2f{}) mMoveDragActor.reset();
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
		if (mContext.viewSettings.requestContent)
		{
			mShowContent = true;
			ImGui::SetNextWindowFocus();
			mContext.viewSettings.requestContent = false;
		}
		if (mShowContent) BuildContentPanel();
		BuildDataPanel();
		BuildExportDialog();
		BuildSceneDialogs();
	}

	bool EditorApplication::HandleApplicationEvent(const sf::Event& event)
	{
		ImGui::SFML::ProcessEvent(GetRenderWindow(), event);
		if (event.is<sf::Event::Closed>())
		{
			RequestScene("exit");
			return true;
		}
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
		if (const auto* key = event.getIf<sf::Event::KeyPressed>(); key &&
			key->code == sf::Keyboard::Key::F && !io.WantTextInput &&
			(overImage || mViewportWindowFocused))
		{
			mContext.FocusSelection();
			mConsumeFocusKey = true;
			return true;
		}
		if (const auto* key = event.getIf<sf::Event::KeyReleased>(); key &&
			key->code == sf::Keyboard::Key::F && mConsumeFocusKey)
		{
			mConsumeFocusKey = false;
			return true;
		}
		const bool editorCamera = GetCurrentWorld() && GetCurrentWorld()->GetViewOverride().has_value();
		if (const auto* key = event.getIf<sf::Event::KeyPressed>(); key && !io.WantTextInput &&
			!key->control && (key->code == sf::Keyboard::Key::W || key->code == sf::Keyboard::Key::E)) return true;
		if (const auto* wheel = event.getIf<sf::Event::MouseWheelScrolled>(); wheel && editorCamera && overImage && !mMoveDragActor)
		{
			const auto pixel = MapWindowToTexturePixel(mousePixel, mViewportImageRect, mViewport.GetSize());
			mCamera.ZoomAt(std::pow(1.1f, -wheel->delta),
				mViewport.MapPixelToCoords(pixel, GetCurrentWorld()->GetWorldView()));
			mContext.ApplyCamera();
			return true;
		}
		if (const auto* button = event.getIf<sf::Event::MouseButtonPressed>(); button && editorCamera && overImage && !mMoveDragActor &&
			(button->button == sf::Mouse::Button::Middle || button->button == sf::Mouse::Button::Right))
		{
			mCameraPanButton = button->button;
			mCameraPanPixel = mousePixel;
			return true;
		}
		if (const auto* button = event.getIf<sf::Event::MouseButtonReleased>(); button &&
			mCameraPanButton == button->button)
		{
			mCameraPanButton.reset();
			return true;
		}
		if (event.is<sf::Event::MouseMoved>() && mCameraPanButton)
		{
			if (editorCamera && overImage)
			{
				const auto previous = MapWindowToTexturePixel(mCameraPanPixel, mViewportImageRect, mViewport.GetSize());
				const auto pixel = MapWindowToTexturePixel(mousePixel, mViewportImageRect, mViewport.GetSize());
				const auto view = GetCurrentWorld()->GetWorldView();
				mCamera.Pan(mViewport.MapPixelToCoords(previous, view) - mViewport.MapPixelToCoords(pixel, view));
				mContext.ApplyCamera();
			}
			mCameraPanPixel = mousePixel;
			return true;
		}
		if (event.is<sf::Event::MouseMovedRaw>() && mCameraPanButton) return true;
		if (event.is<sf::Event::FocusLost>())
		{
			mCameraPanButton.reset();
			mMoveDragActor.reset();
		}
		// The paused or editor-camera viewport owns left clicks for selection and gizmos.
		// The live game camera still passes clicks through to gameplay.
		// Remember the press so its release is consumed even if the cursor leaves the image.
		if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>())
		{
			const ly::World* world = GetCurrentWorld();
			if (pressed->button == sf::Mouse::Button::Left && world && (world->IsPaused() || editorCamera) &&
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
		if (mouseEvent && mMoveDragActor) return true;
		if (const auto* pressed = event.getIf<sf::Event::KeyPressed>(); pressed && pressed->control &&
			!io.WantTextInput && (pressed->code == sf::Keyboard::Key::Z || pressed->code == sf::Keyboard::Key::Y)) return true;
		// HUD widgets hit-test the event's own position, but in the editor they are drawn into the scaled viewport
		// texture. Over the image, send the world a copy whose positions are texture pixels and consume the original,
		// which Application would otherwise dispatch in window pixels.
		if (overImage && (event.is<sf::Event::MouseButtonPressed>() || event.is<sf::Event::MouseButtonReleased>() ||
			event.is<sf::Event::MouseMoved>() || event.is<sf::Event::MouseWheelScrolled>()))
		{
			sf::Event remapped = event;
			const auto toTexture = [this](sf::Vector2i windowPixel)
			{
				return MapWindowToTexturePixel(windowPixel, mViewportImageRect, mViewport.GetSize());
			};
			if (auto* pressed = remapped.getIf<sf::Event::MouseButtonPressed>()) pressed->position = toTexture(pressed->position);
			if (auto* released = remapped.getIf<sf::Event::MouseButtonReleased>()) released->position = toTexture(released->position);
			if (auto* moved = remapped.getIf<sf::Event::MouseMoved>()) moved->position = toTexture(moved->position);
			if (auto* wheel = remapped.getIf<sf::Event::MouseWheelScrolled>()) wheel->position = toTexture(wheel->position);
			if (!GameApplication::HandleApplicationEvent(remapped))
			{
				if (ly::World* world = GetCurrentWorld())
				{
					world->DispatchEvent(remapped);
				}
			}
			return true;
		}
		return GameApplication::HandleApplicationEvent(event);
	}
}
