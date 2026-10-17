#pragma once

#include <framework/GameModule.h>
#include <framework/Actor.h>
#include <framework/ActorComponent.h>
#include <framework/ActorRef.h>

namespace blank
{
	class BlankMovement final : public vega::ActorComponent
	{
		VEGA_CLASS(BlankMovement)
	public:
		explicit BlankMovement(vega::Actor& owner);
		void TickComponent(float deltaTime) override;
		float GetSpeed() const { return mSpeed; }
		void SetSpeed(float speed) { mSpeed = speed; }
	private:
		float mSpeed = 240.f;
	};

	class BlankPlayer final : public vega::Actor
	{
		VEGA_CLASS(BlankPlayer)
	public:
		explicit BlankPlayer(vega::World* world);
		float GetSize() const;
		void SetSize(float size);
		const vega::ActorRef& GetTarget() const { return mTarget; }
		void SetTarget(const vega::ActorRef& value) { mTarget = value; }
		const std::vector<sf::Vector2f>& GetPatrol() const { return mPatrol; }
		void SetPatrol(const std::vector<sf::Vector2f>& value) { mPatrol = value; }
	private:
		vega::ActorRef mTarget;
		std::vector<sf::Vector2f> mPatrol;
	};

	class BlankGameModule final : public vega::GameModule, public vega::IModuleSetup, public vega::IPlayerProvider
	{
	public:
		const char* Name() const override { return "Blank"; }
		void RegisterTypes() override;
		void ConfigureProject(const std::filesystem::path& file) override;
		void Initialize(vega::Application& application, bool editor) override;
		std::shared_ptr<vega::World> CreateLevel(vega::Application& application,
			const nlohmann::json& scene, bool edit) override;
		std::shared_ptr<vega::Actor> PlayerActor(vega::World& world) override;
	private:
		std::filesystem::path mRoot{ "assets" };
	};
}
