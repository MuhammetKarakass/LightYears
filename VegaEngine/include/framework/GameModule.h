#pragma once

#include <framework/Application.h>
#include <framework/World.h>
#include <type_traits>
#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace vega
{
	class EditorExtensions;

	struct ActorDisplayInfo
	{
		bool spawnMarker = false;
		std::string label;
	};

	// A module only has to identify itself, register its types and create worlds.
	// Capabilities are owned by that same module and must outlive its worlds.
	class GameModule
	{
	public:
		virtual ~GameModule() = default;
		virtual const char* Name() const = 0;
		virtual void RegisterTypes() = 0;
		virtual std::shared_ptr<vega::World> CreateLevel(vega::Application& application,
			const nlohmann::json& scene, bool edit) = 0;
	};

	class IModuleSetup
	{
	public:
		virtual ~IModuleSetup() = default;
		virtual void ConfigureProject(const std::filesystem::path&) {}
		virtual void Initialize(vega::Application&, bool editor) = 0;
	};
	class IPlayHooks
	{
	public:
		virtual ~IPlayHooks() = default;
		virtual void BeginPlay(vega::World&) {}
		virtual void EndPlay(vega::World&) {}
	};
	class IPlayerProvider
	{
	public:
		virtual ~IPlayerProvider() = default;
		virtual std::shared_ptr<vega::Actor> PlayerActor(vega::World&) = 0;
	};
	class IActorDisplayInfo
	{
	public:
		virtual ~IActorDisplayInfo() = default;
		virtual ActorDisplayInfo DisplayInfo(const vega::Actor&) const = 0;
	};
	class IEditorProvider
	{
	public:
		virtual ~IEditorProvider() = default;
		// Extensions retain the host's EngineContext; use extensions.Engine() for
		// schema registration so a different ambient binding cannot select the host.
		virtual void RegisterEditorExtensions(EditorExtensions&) = 0;
	};
	class ISceneSettingsHandler
	{
	public:
		virtual ~ISceneSettingsHandler() = default;
		virtual void ApplySceneSettings(const nlohmann::json&) = 0;
		virtual std::string SceneBackdropState() const = 0;
	};
	class IAppLoopHooks
	{
	public:
		virtual ~IAppLoopHooks() = default;
		virtual void Tick(float) {}
		virtual bool HandleEvent(const sf::Event&) { return false; }
		virtual void AfterRender() {}
		virtual int ExitCode() const { return 0; }
	};
	class IContentReload
	{
	public:
		virtual ~IContentReload() = default;
		// Absolute paths in the selected authoring root; called on the editor thread.
		virtual void ReloadContent(const std::vector<std::filesystem::path>&) = 0;
	};

	// Keep RTTI at the module boundary, including null and const queries.
	template<typename Capability, typename Module>
	auto QueryCapability(Module* module)
		-> std::conditional_t<std::is_const_v<Module>, const Capability*, Capability*>
	{
		using Result = std::conditional_t<std::is_const_v<Module>, const Capability*, Capability*>;
		return dynamic_cast<Result>(module);
	}
}
