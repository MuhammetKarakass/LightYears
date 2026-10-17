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
#include <algorithm>

namespace blank
{
	BlankMovement::BlankMovement(vega::Actor& owner) : ActorComponent{ owner }
	{
		mCanEverTick = true;
	}

	void BlankMovement::TickComponent(float deltaTime)
	{
		const auto down = [](const char* key) { return vega::script::ScriptRuntime::Get().ReadKey(key); };
		const float x = float(down("D") || down("Right")) - float(down("A") || down("Left"));
		const float y = float(down("S") || down("Down")) - float(down("W") || down("Up"));
		GetOwner().AddActorLocationOffset({ x * mSpeed * deltaTime, y * mSpeed * deltaTime });
		if (const auto& bounds = GetOwner().GetWorld()->GetSceneBounds())
		{
			auto position = GetOwner().GetActorLocation();
			position.x = std::clamp(position.x, bounds->position.x, bounds->position.x + bounds->size.x);
			position.y = std::clamp(position.y, bounds->position.y, bounds->position.y + bounds->size.y);
			GetOwner().SetActorLocation(position);
		}
	}

	BlankPlayer::BlankPlayer(vega::World* world) : Actor{ world, "textures/tile.png" }
	{
		AddComponent<BlankMovement>();
	}
	float BlankPlayer::GetSize() const { return FindComponent<vega::SpriteComponent>()->GetScale().x; }
	void BlankPlayer::SetSize(float size) { FindComponent<vega::SpriteComponent>()->SetScale({ size, size }); }

	VEGA_REFLECT(BlankMovement, vega::ActorComponent)
	{
		VEGA_ADDABLE();
		VEGA_PROPERTY("Speed", GetSpeed, SetSpeed).Range(0.f, 2000.f);
	}
	VEGA_REFLECT(BlankPlayer, vega::Actor)
	{
		VEGA_SPAWNABLE();
		VEGA_REQUIRES(SpriteComponent);
		VEGA_REQUIRES(BlankMovement);
		VEGA_PROPERTY("Size", GetSize, SetSize).Range(0.1f, 10.f);
		VEGA_ACTOR_REF("Target", GetTarget, SetTarget).RequiredType("BlankPlayer");
		VEGA_LIST("Patrol", GetPatrol, SetPatrol).Speed(1.f);
	}

	namespace
	{
		class BlankWorld final : public vega::World
		{
		public:
			BlankWorld(vega::Application* app, const nlohmann::json& scene, bool edit)
				: World{ app }, mEdit{ edit }
			{
				const auto loaded = vega::SceneSerializer::Load(*this, scene);
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
		vega::reflect::RegisterEngineTypes();
		VEGA_REGISTER(BlankMovement);
		VEGA_REGISTER(BlankPlayer);
		vega::script::RegisterScriptTypes();
	}
	void BlankGameModule::ConfigureProject(const std::filesystem::path& file)
	{
		const auto project = vega::ProjectSettings::Load(file);
		mRoot = std::filesystem::absolute(file.parent_path() / project.editor.value("assetRoot", std::string{ "." }));
	}
	void BlankGameModule::Initialize(vega::Application&, bool)
	{
#ifdef _MSC_VER
		for (int i = 1; i + 1 < __argc; ++i)
			if (std::string{ __argv[i] } == "--asset-root") mRoot = __argv[++i];
#endif
		vega::AssetManager::GetAssetManager().SetAssetRootDirectory(mRoot.generic_string() + "/");
		vega::script::SetScriptRoot(mRoot);
		vega::script::ScriptRuntime::Get().SetPlayerResolver([this](vega::World& world) { return PlayerActor(world); });
	}
	std::shared_ptr<vega::World> BlankGameModule::CreateLevel(vega::Application& application,
		const nlohmann::json& scene, bool edit)
	{
		return std::make_shared<BlankWorld>(&application, scene, edit);
	}
	std::shared_ptr<vega::Actor> BlankGameModule::PlayerActor(vega::World& world)
	{
		for (const auto& weak : world.GetActorsByTypeIncludingPending<BlankPlayer>())
			if (auto actor = weak.lock())
				if (auto* identity = actor->FindComponent<vega::SceneEntityComponent>(); identity && identity->name == "Player") return actor;
		return {};
	}
}
