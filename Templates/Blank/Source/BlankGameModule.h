#pragma once

#include <framework/GameModule.h>
#include <framework/Actor.h>
#include <framework/ActorComponent.h>

namespace blank
{
	class BlankMovement final : public ly::ActorComponent
	{
		VEGA_CLASS(BlankMovement)
	public:
		explicit BlankMovement(ly::Actor& owner);
		void TickComponent(float deltaTime) override;
		float GetSpeed() const { return mSpeed; }
		void SetSpeed(float speed) { mSpeed = speed; }
	private:
		float mSpeed = 240.f;
	};

	class BlankPlayer final : public ly::Actor
	{
		VEGA_CLASS(BlankPlayer)
	public:
		explicit BlankPlayer(ly::World* world);
		float GetSize() const;
		void SetSize(float size);
	};

	class BlankGameModule final : public vega::GameModule
	{
	public:
		const char* Name() const override { return "Blank"; }
		void RegisterTypes() override;
		void ConfigureProject(const std::filesystem::path& file) override;
		void Initialize(ly::Application& application, bool editor) override;
		std::shared_ptr<ly::World> CreateLevel(ly::Application& application,
			const nlohmann::json& scene, bool edit) override;
		std::shared_ptr<ly::Actor> PlayerActor(ly::World& world) override;
	private:
		std::filesystem::path mRoot{ "assets" };
	};
}
