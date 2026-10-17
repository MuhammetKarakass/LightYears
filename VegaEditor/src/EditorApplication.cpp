#include "editor/EditorApplication.h"
#include "editor/EditorTheme.h"

#include <framework/World.h>
#include <framework/Actor.h>
#include <framework/scene/Scene.h>
#include <framework/scene/SceneSettings.h>
#include <framework/scene/SceneEntityComponent.h>
#include <framework/scene/PrefabLibrary.h>
#include <script/ScriptRuntime.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui-SFML.h>

#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <string>
#ifdef _MSC_VER
#include <crtdbg.h>
#endif

namespace vega
{
	namespace
	{
		bool Within(const std::filesystem::path& path, const std::filesystem::path& root)
		{
			const auto relative = path.lexically_relative(root);
			if (relative.empty()) return path == root;
			for (const auto& part : relative) if (part == "..") return false;
			return !relative.is_absolute();
		}
	}
	EditorApplication::EditorApplication(std::unique_ptr<GameModule> module)
		: EditorApplication{ std::move(module), EditorProject::FromCommandLine() }
	{
	}

	EditorApplication::EditorApplication(std::unique_ptr<GameModule> module, EditorProject project)
		: ly::Application{ { project.settings.windowWidth, project.settings.windowHeight }, 64, "Vega Editor", sf::Style::Close | sf::Style::Titlebar },
		mProject{ std::move(project) },
		mModule{ std::move(module) }, mViewport{ GetRenderWindow() }, mContentAuthoring{ GetEngineContext() }
	{
		if (!mModule) throw std::runtime_error("Editor requires a game module");
		bool unattended = false;
#ifdef _MSC_VER
		for (int index = 1; index < __argc; ++index) if (std::string{ __argv[index] } == "--editor-script") unattended = true;
#endif
		const bool sourceReadOnly = unattended && Within(mDocument.Root(), VEGA_SOURCE_ROOT) && !Within(mDocument.Root(), VEGA_BUILD_ROOT);
		if (!sourceReadOnly && AssetMovePlan::RecoverPending(mDocument.Root(), mProject.file, mAssetRecoveryError) && std::filesystem::exists(mProject.file))
			mProject.settings = ly::ProjectSettings::Load(mProject.file);
		mProject.settings.ApplyCollisionLayers(GetEngineContext());
		if (auto* setup = QueryCapability<IModuleSetup>(mModule.get())) setup->ConfigureProject(mProject.file);
		RegisterGameModule(*mModule, true);
		if (auto* provider = QueryCapability<IEditorProvider>(mModule.get())) provider->RegisterEditorExtensions(mExtensions);
		mContext.services = mPrefabContext.services = &GetEngineContext();
		mContext.extensions = mPrefabContext.extensions = &mExtensions;
		std::snprintf(mExportDirectory, sizeof(mExportDirectory), "%s", mProject.exportDir.u8string().c_str());
		if (!ImGui::SFML::Init(GetRenderWindow()))
		{
			ImGui::SFML::Shutdown();
			throw std::runtime_error("Cannot initialize the editor UI");
		}
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		mDiagnostics.InstallSettings();
		InstallWorldSettingsLayout();
		if (ly::debug::Log::GetMinimumLevel() > ly::debug::LogLevel::Info)
			ly::debug::Log::SetMinimumLevel(ly::debug::LogLevel::Info);
		ly::debug::Profiler::SetEnabled(false);
		ApplyEditorTheme(mProject.font);
		// The default dock layout is built only when the saved layout is missing, so panels added later would open
		// floating over an older layout. Versioning the file name rebuilds the default once; bump it whenever the
		// default layout or its reserved toolbar/status area changes.
		ImGui::GetIO().IniFilename = "vega_editor_layout_v5.ini";
		// Image clicks select actors; panel dragging belongs on the title/tab bars.
		ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;
		GetRenderWindow().setTitle("Vega Editor");
		std::filesystem::path startupScene = mProject.settings.startupScene.empty()
			? "scenes/sandbox.scene.json" : mProject.settings.startupScene;
		std::string scriptPath;
		bool scriptRequested = false;
#ifdef _MSC_VER
		for (int index = 1; index < __argc; ++index)
		{
			if (std::string{ __argv[index] } == "--editor-script")
			{
				scriptRequested = true;
				scriptPath = index + 1 < __argc ? __argv[index + 1] : "";
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
		mExportLayout.assetRoot = mDocument.Root();
		// Unattended scripts share an executable directory, but must neither race
		// each other's layout writes nor replace the user's saved workspace.
		if (scriptRequested) ImGui::GetIO().IniFilename = nullptr;
		mLuaTypesPath = mDocument.Root() / ".vega" / "vega.lua";
		// Existing automation can read source assets. Generated files still belong
		// in the build tree when no disposable authoring root was supplied.
		if (scriptRequested && mDocument.Root() == mProject.sourceAssetRoot)
			mLuaTypesPath = std::filesystem::path{ VEGA_BUILD_ROOT } / "VegaEditor/.vega/vega.lua";
		RegenerateLuaTypes();
		script::ScriptRuntime::Get(GetEngineContext()).SetRoot(mDocument.Root());
#ifdef _MSC_VER
		if (scriptRequested)
		{
			// An assertion in unattended script mode must report to CTest, not
			// wait forever for a Windows debug dialog that nobody can dismiss.
			_set_error_mode(_OUT_TO_STDERR);
			_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
			_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
			_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
		}
#endif
		GetEngineContext().Prefabs().SetRoot(mDocument.Root());
		if (scriptRequested) mScript = std::make_unique<EditorScript>(scriptPath, mExportLayout, mExtensions.commands);
		// Source-root automation stays read-only, just like generated Lua tooling.
		// Disposable project copies get the complete autosave/recovery lifecycle.
		const bool sourceAutomation = scriptRequested && Within(mDocument.Root(), VEGA_SOURCE_ROOT) && !Within(mDocument.Root(), VEGA_BUILD_ROOT);
		if (!sourceAutomation)
		{
			if (mAutosaves.Begin(mDocument.Root()))
			{
				mRecoveryEntries = mAutosaves.Entries(true);
				mRecoveryRequested = !mRecoveryEntries.empty();
			}
			else mAutosaveError = mAutosaves.Error();
		}
		mContentWatch.Begin(mDocument.Root());
		mNextAutosave = std::chrono::steady_clock::now() + std::chrono::seconds{ mProject.settings.AutosaveSeconds() };
		mContext.autosaveCommand = mPrefabContext.autosaveCommand = [this](const EditorScriptCommand& command, std::string& detail)
		{
			return ExecuteAutosaveCommand(command, detail);
		};
		mContext.assetCommand = mPrefabContext.assetCommand = [this](const EditorScriptCommand& command, std::string& detail)
		{
			return ExecuteAssetCommand(command, detail);
		};
		mContext.document = &mDocument;
		mContext.contentCommand = [this](const EditorScriptCommand& command, std::string& detail)
		{
			return mContentAuthoring.Execute(mContext, mHistory, mDocument.Root(), command, detail);
		};
		mContext.sceneSettings = &mSceneSettings;
		mContext.backdropState = [this]() { const auto* handler = QueryCapability<ISceneSettingsHandler>(mModule.get()); return handler ? handler->SceneBackdropState() : std::string{}; };
		mContext.applySettings = [this](const nlohmann::json& authored)
		{
			// Invalid legacy values stay visible and serializable. Presentation gets
			// safe effective values so undo and reset can always restore the document.
			auto effective = authored;
			for (const auto& field : GetEngineContext().SceneSettings().Fields())
			{
				const auto value = GetEngineContext().SceneSettings().Value(authored, field.key);
				effective[field.key] = field.Validate(value).empty() ? value : field.defaultValue;
			}
			if (mContext.world)
			{
				mContext.world->SetSceneBounds(ly::ReadSceneBounds(effective, GetEngineContext().SceneSettings()));
				ApplySceneSettings(effective);
			}
		};
		mContext.console = mPrefabContext.console = &mDiagnostics.capture.Buffer();
		mContext.clearConsole = mPrefabContext.clearConsole = [this]() { mDiagnostics.Clear(); };
		mContext.diagnosticsVisible = mPrefabContext.diagnosticsVisible = [this](bool visible)
		{
			mDiagnostics.showConsole = mDiagnostics.showProfiler = visible;
			if (visible) mDiagnostics.FocusConsole();
		};
		mContext.contentChanged = [this]() { RefreshContent(); };
		mPrefabContext.contentChanged = mContext.contentChanged;
		mContext.prefabAction = [this](const std::string& action, const std::string& path)
		{
			if (mContext.playMode || mContext.transitioning) return false;
			if (action == "save") return SavePrefab();
			if (PrefabDirty()) { mContext.sceneError = "Unsaved prefab: save before replacing it"; return false; }
			if (action == "open") return OpenPrefab(path);
			if (action == "close") { ClosePrefab(); return true; }
			return false;
		};
		mPrefabContext.prefabAction = mContext.prefabAction;
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
		for (const auto& tool : mExtensions.tools) tool->Initialize(mContext);
		OpenScene(startupScene, true);
		RefreshContent();
	}

	EditorApplication::~EditorApplication()
	{
		UnregisterGameModule();
		for (const auto& tool : mExtensions.tools) tool->Shutdown();
		// The base application can still own a world when editor members are destroyed.
		if (ly::World* world = GetCurrentWorld())
		{
			world->SetRenderViewport(nullptr);
			world->SetViewOverride(std::nullopt);
		}
		ImGui::SFML::Shutdown();
	}

	void EditorApplication::RegenerateLuaTypes()
	{
		std::string error;
		if (!script::GenerateLuaTypes(GetEngineContext(), mLuaTypesPath, error))
			ly::debug::Log::Write(ly::debug::LogLevel::Error, ly::debug::LogChannel::Editor, LY_SOURCE_LOCATION, error);
	}

	void EditorApplication::Render()
	{
		mContext.ResetActorCache();
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
					mSavedSceneRevision = mContext.sceneRevision;
				}
				mSelection.Restore(mRestoreSelection);
				mSelection.Rebind([this](const std::string& guid) { return mContext.ResolveGuid(guid); });
				mRestoreSelection.Clear();
			}
			mCameraPanButton.reset();
		}
		mContext.world = current;
		mViewport.ResizeToWindow();
		for (const auto& tool : mExtensions.tools) tool->Update(mContext, mViewport.GetSize(), mContext.playMode || mPrefabWorld != nullptr);
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
			const auto primary = mSelection.Get();
			for (const auto& actor : mSelection.GetAll())
			{
				if (actor->GetWorld() == world)
				{
					const sf::View previousView = texture.getView();
					const sf::View worldView = world->GetWorldView();
					const sf::FloatRect bounds = actor->GetActorGlobalBounds();
					sf::RectangleShape outline{ bounds.size };
					outline.setPosition(bounds.position);
					outline.setFillColor(sf::Color::Transparent);
					outline.setOutlineColor(actor == primary ? sf::Color{ 255, 222, 116 } : sf::Color{ 51, 199, 202 });
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
		PollAutosave();
		PollContentReload();
		ImGui::SFML::Render(window);
		if (mScript && !mContext.transitioning && mScript->Step(mPrefabWorld ? mPrefabContext : mContext, mPrefabWorld ? mPrefabHistory : mHistory, window)) QuitApplication();
		// The world is drawn into the viewport texture, so the window holds the finished frame only once the UI is on
		// it (and only then is the window's framebuffer the active one); run the --visual-capture step after that.
		if (auto* hooks = QueryCapability<IAppLoopHooks>(mModule.get())) hooks->AfterRender();
	}

	int EditorApplication::GetExitCode() const
	{
		const auto* hooks = QueryCapability<IAppLoopHooks>(mModule.get());
		return mScript ? mScript->ExitCode() : hooks ? hooks->ExitCode() : 0;
	}

	void EditorApplication::BuildUI()
	{
		LY_PROFILE_SCOPE("ImGui Build");
		auto& activeHistory = mExtensions.History() ? *mExtensions.History() : mPrefabWorld ? mPrefabHistory : mHistory;
		mHistory.PruneInvalidCommands();
		if (!mShowContent) mContentFocused = false;
		const ImGuiIO& io = ImGui::GetIO();
		if (io.KeyCtrl && !io.WantTextInput && !mContext.playMode)
		{
			if (ImGui::IsKeyPressed(ImGuiKey_Z, false))
			{
				if (io.KeyShift) activeHistory.Redo();
				else activeHistory.Undo();
			}
			if (ImGui::IsKeyPressed(ImGuiKey_Y, false)) activeHistory.Redo();
		}
		if (!mContext.playMode && io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false) && !mDataFocused && !mExtensions.Active())
		{
			if (mPrefabWorld) SavePrefab(); else SaveScene();
		}
		if (!mPrefabWorld && !mContext.playMode && !io.WantTextInput && !mExtensions.Active() && !mContentFocused)
		{
			if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D, false)) DuplicateSceneEntity(mContext, mHistory);
			if (ImGui::IsKeyPressed(ImGuiKey_Delete, false)) DeleteSelection(mContext, mHistory);
			if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C, false)) CopySelection(mContext);
			if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V, false) && mContext.world)
				{
				const auto mouse = ImGui::GetMousePos();
				const auto origin = ImGui::GetMainViewport()->Pos;
				const auto pixel = MapWindowToTexturePixel({ static_cast<int>(mouse.x - origin.x), static_cast<int>(mouse.y - origin.y) }, mViewportImageRect, mViewport.GetSize());
				PasteSelection(mContext, mHistory, mViewportImageHovered ? mViewport.MapPixelToCoords(pixel, mContext.world->GetWorldView()) : mCamera.GetView().getCenter());
			}
			if (ImGui::IsKeyPressed(ImGuiKey_Escape, false) && !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel))
			{
				mSelection.Clear();
				mMarquee = false;
				mMoveDragActor.reset();
			}
			if (ImGui::IsKeyPressed(ImGuiKey_F2, false)) BeginRename();
		}
		const std::string sceneTitle = (mScenePath.empty() ? "Untitled" : mScenePath.filename().u8string()) + (SceneDirty() ? " *" : "");
		const std::string title = "Vega Editor - " + sceneTitle + (mContext.playMode ? " - PLAYING" : " - EDIT");
		if (title != mWindowTitle)
		{
			GetRenderWindow().setTitle(title);
			mWindowTitle = title;
		}
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
					std::snprintf(mScenePathInput, sizeof(mScenePathInput), "%s", mScenePath.empty() ? "scenes/untitled.scene.json" : mScenePath.generic_u8string().c_str());
					mSaveSceneDialog = true;
				}
				ImGui::EndDisabled();
				ImGui::Separator();
				if (ImGui::MenuItem("Project Settings...")) mProjectDialogRequested = true;
				if (ImGui::MenuItem("Recover Autosaves...", nullptr, false, mAutosaves.Enabled()))
				{
					mRecoveryEntries = mAutosaves.Entries();
					mRecoveryRequested = true;
				}
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
				const std::string undoLabel = activeHistory.CanUndo() ? "Undo " + activeHistory.UndoName() : "Undo";
				const std::string redoLabel = activeHistory.CanRedo() ? "Redo " + activeHistory.RedoName() : "Redo";
				if (ImGui::MenuItem(undoLabel.c_str(), "Ctrl+Z", false, !mContext.playMode && activeHistory.CanUndo())) activeHistory.Undo();
				if (ImGui::MenuItem(redoLabel.c_str(), "Ctrl+Y / Ctrl+Shift+Z", false, !mContext.playMode && activeHistory.CanRedo())) activeHistory.Redo();
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("View"))
			{
				ImGui::MenuItem("Viewport", nullptr, &mShowViewport);
				ImGui::MenuItem("Hierarchy", nullptr, &mShowHierarchy);
				ImGui::MenuItem("Inspector", nullptr, &mShowInspector);
				ImGui::MenuItem("Stats", nullptr, &mShowStats);
				if (ImGui::MenuItem("Console", nullptr, &mDiagnostics.showConsole) && mDiagnostics.showConsole) mDiagnostics.FocusConsole();
				ImGui::MenuItem("Profiler", nullptr, &mDiagnostics.showProfiler);
				ImGui::MenuItem("Content", nullptr, &mShowContent);
				ImGui::Separator();
				ImGui::MenuItem("World Settings", nullptr, &mShowWorldSettings);
				for (const auto& tool : mExtensions.tools) tool->ViewMenu();
				ImGui::MenuItem("Content Authoring", nullptr, &mContentAuthoring.visible);

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
			if (ImGui::BeginMenu("Scripts"))
			{
				if (ImGui::MenuItem("Regenerate Lua Types")) RegenerateLuaTypes();
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
			ImGuiID content = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Down, 0.42f, nullptr, &centre);
			const ImGuiID profiler = ImGui::DockBuilderSplitNode(content, ImGuiDir_Right, 0.50f, nullptr, &content);
			ImGui::DockBuilderDockWindow("Console", content);
			ImGui::DockBuilderDockWindow("Profiler", profiler);
			ImGui::DockBuilderDockWindow("Viewport", centre);
			ImGui::DockBuilderDockWindow("Hierarchy", left);
			ImGui::DockBuilderDockWindow("Inspector", right);
			ImGui::DockBuilderDockWindow("World Settings", right);
			for (const auto& name : mExtensions.inspectorWindows) ImGui::DockBuilderDockWindow(name.c_str(), right);
			ImGui::DockBuilderDockWindow("Stats", stats);
			ImGui::DockBuilderDockWindow("Content", content);
			ImGui::DockBuilderDockWindow("Data Editor###DataEditor", content);
			ImGui::DockBuilderDockWindow("Content Authoring", centre);
			ImGui::DockBuilderDockWindow("Content validation", content);
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
		if (mViewportImageRect.size == sf::Vector2f{}) { mMoveDragActor.reset(); mMarquee = false; }
		mSelectionClickPending = false;
		if (mShowHierarchy)
		{
			BuildHierarchyPanel();
		}
		if (mShowInspector)
		{
			BuildInspectorPanel();
		}
		if (mShowWorldSettings) BuildWorldSettingsPanel();
		for (const auto& panel : mExtensions.panels) panel(mPrefabWorld ? mPrefabContext : mContext, mPrefabWorld ? mPrefabHistory : mHistory);
		if (mShowWorldSettings != mSavedWorldSettingsVisible)
		{
			mSavedWorldSettingsVisible = mShowWorldSettings;
			ImGui::MarkIniSettingsDirty();
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
		if (mDiagnostics.showConsole) mDiagnostics.BuildConsole();
		if (mDiagnostics.showProfiler) mDiagnostics.BuildProfiler();
		mDiagnostics.TrackSettings();
		BuildDataPanel();
		mContentAuthoring.Build(mContext, mHistory, mDocument.Root());
		BuildExportDialog();
		BuildSceneDialogs();
		BuildPrefabDialogs();
		BuildProjectDialog();
		BuildRecoveryDialog();
		BuildAssetDialogs();
		mDiagnostics.ReportError("Autosave", mAutosaveError);
		mDiagnostics.ReportError("Asset recovery", mAssetRecoveryError);
		mDiagnostics.ReportError("Scene", mContext.sceneError);
		mDiagnostics.ReportError("Prefab", mPrefabContext.sceneError);
		mDiagnostics.ReportError("Project", mProjectError);
		mDiagnostics.ReportError("Content", mContentError);
		mDiagnostics.ReportError("Document", mDocument.Error());
		mDiagnostics.ReportError("Export", mExportResult ? mExportResult->error : std::string{});
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
			mMarquee = false;
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
			auto* hooks = QueryCapability<IAppLoopHooks>(mModule.get());
			if (!hooks || !hooks->HandleEvent(remapped))
			{
				if (ly::World* world = GetCurrentWorld())
				{
					world->DispatchEvent(remapped);
				}
			}
			return true;
		}
		auto* hooks = QueryCapability<IAppLoopHooks>(mModule.get());
		return hooks && hooks->HandleEvent(event);
	}
}
