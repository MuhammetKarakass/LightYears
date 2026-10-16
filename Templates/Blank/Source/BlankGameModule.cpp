#include "BlankGameModule.h"
#include <framework/AssetManager.h>
#include <framework/ProjectSettings.h>
#include <framework/SpriteComponent.h>
#include <framework/scene/Scene.h>
#include <framework/scene/SceneEntityComponent.h>
#include <framework/reflection/Reflection.h>
#include <script/ScriptRuntime.h>
#include <cstdlib>
#include <stdexcept>

namespace blank
{
	BlankMovement::BlankMovement(ly::Actor& owner) : ActorComponent{ owner }
	{
		mCanEverTick = true;
	}

	void BlankMovement::TickComponent(float deltaTime)
	{
		const auto down = [](const char* key) { return vega::script::ScriptRuntime::Get().ReadKey(key); };
		const float x = float(down("D") || down("Right")) - float(down("A") || down("Left"));
		const float y = float(down("S") || down("Down")) - float(down("W") || down("Up"));
		GetOwner().AddActorLocationOffset({ x * mSpeed * deltaTime, y * mSpeed * deltaTime });
	}

	BlankPlayer::BlankPlayer(ly::World* world) : Actor{ world, "textures/tile.png" }
	{
		AddComponent<BlankMovement>();
	}
	float BlankPlayer::GetSize() const { return FindComponent<ly::SpriteComponent>()->GetScale().x; }
	void BlankPlayer::SetSize(float size) { FindComponent<ly::SpriteComponent>()->SetScale({ size, size }); }

	VEGA_REFLECT(BlankMovement, ly::ActorComponent)
	{
		VEGA_ADDABLE();
		VEGA_PROPERTY("Speed", GetSpeed, SetSpeed).Range(0.f, 2000.f);
	}
	VEGA_REFLECT(BlankPlayer, ly::Actor)
	{
		VEGA_SPAWNABLE();
		VEGA_REQUIRES(SpriteComponent);
		VEGA_REQUIRES(BlankMovement);
		VEGA_PROPERTY("Size", GetSize, SetSize).Range(0.1f, 10.f);
	}

	namespace
	{
		class BlankWorld final : public ly::World
		{
		public:
			BlankWorld(ly::Application* app, const nlohmann::json& scene, bool edit)
				: World{ app }, mEdit{ edit }
			{
				const auto loaded = ly::SceneSerializer::Load(*this, scene);
				if (!loaded.warnings.empty()) throw std::runtime_error(loaded.warnings.front());
				SetPaused(edit);
			}
			bool IsEditMode() const override { return mEdit; }
		private:
			bool mEdit;
		};
	}

	void BlankGameModule::RegisterTypes()
	{
		ly::reflect::RegisterEngineTypes();
		VEGA_REGISTER(BlankMovement);
		VEGA_REGISTER(BlankPlayer);
		vega::script::RegisterScriptTypes();
	}
	void BlankGameModule::ConfigureProject(const std::filesystem::path& file)
	{
		const auto project = ly::ProjectSettings::Load(file);
		mRoot = std::filesystem::absolute(file.parent_path() / project.editor.value("assetRoot", std::string{ "." }));
	}
	void BlankGameModule::Initialize(ly::Application&, bool)
	{
#ifdef _MSC_VER
		for (int i = 1; i + 1 < __argc; ++i)
			if (std::string{ __argv[i] } == "--asset-root") mRoot = __argv[++i];
#endif
		ly::AssetManager::GetAssetManager().SetAssetRootDirectory(mRoot.generic_string() + "/");
		vega::script::SetScriptRoot(mRoot);
		vega::script::ScriptRuntime::Get().SetPlayerResolver([this](ly::World& world) { return PlayerActor(world); });
	}
	std::shared_ptr<ly::World> BlankGameModule::CreateLevel(ly::Application& application,
		const nlohmann::json& scene, bool edit)
	{
		return std::make_shared<BlankWorld>(&application, scene, edit);
	}
	std::shared_ptr<ly::Actor> BlankGameModule::PlayerActor(ly::World& world)
	{
		for (const auto& weak : world.GetActorsByTypeIncludingPending<BlankPlayer>())
			if (auto actor = weak.lock())
				if (auto* identity = actor->FindComponent<ly::SceneEntityComponent>(); identity && identity->name == "Player") return actor;
		return {};
	}
}
