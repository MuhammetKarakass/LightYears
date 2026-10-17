#pragma once

#include <framework/Actor.h>
#include <framework/ActorComponent.h>
#include <framework/AudioManager.h>
#include <framework/ComponentIdentity.h>
#include <framework/EngineContext.h>
#include <framework/reflection/Reflection.h>
#include <nlohmann/json.hpp>
#include <sol/sol.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>
#include <cstdint>
#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace vega::script
{
	using Value = vega::reflect::PropertyValue;
	using Clock = std::chrono::steady_clock;

	namespace detail
	{
		struct ActorHandle
		{
			std::weak_ptr<vega::Actor> actor;
			std::shared_ptr<bool> destroying = std::make_shared<bool>(false);
			std::shared_ptr<vega::Actor> Lock() const
			{
				auto result = actor.lock();
				return result && !result->GetIsPendingDestroy() ? result : nullptr;
			}
		};
		struct ComponentHandle
		{
			ActorHandle owner;
			vega::ComponentIdentity identity;
			vega::ActorComponent* Resolve(const std::shared_ptr<vega::Actor>& actor) const
			{
				return actor ? vega::FindComponentByTypeOccurrence(*actor, identity) : nullptr;
			}
		};
		struct Vector
		{
			float x = 0.f, y = 0.f;
		};
		struct TimerHandle
		{
			std::weak_ptr<bool> active;
			void Cancel() const { if (auto value = active.lock()) *value = false; }
		};
		struct SoundHandle
		{
			std::uint64_t id = 0;
			vega::EngineContext* context = nullptr;
			void Stop() const { if (context && context->HasAudio()) context->Audio().StopSound(id); }
			bool Playing() const { return context && context->HasAudio() && context->Audio().IsSoundPlaying(id); }
		};

		bool Matches(const vega::Actor& actor, const std::string& name);
		nlohmann::json MessageData(const sol::object& object, unsigned int& entries, unsigned int depth = 0);
		sol::object MessageObject(sol::state_view lua, const nlohmann::json& data);
		sol::object ToLua(sol::state_view lua, const Value& value, vega::World* world = nullptr);
		std::optional<Value> FromLua(const sol::object& object);
		bool KeyDown(const ActorHandle& handle, const std::string& name);
		void Bind(sol::state& lua);
	}

	using detail::ActorHandle;
	using detail::ComponentHandle;
	using detail::SoundHandle;
	using detail::TimerHandle;
	using detail::Vector;
}
