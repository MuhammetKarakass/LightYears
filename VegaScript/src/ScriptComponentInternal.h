#pragma once

#include "script/ScriptComponent.h"
#include "script/ScriptRuntime.h"
#include "ScriptBindingsInternal.h"
#include <deque>
#include <filesystem>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace vega::script
{
	struct ScriptComponent::Impl
	{
		ScriptComponent& component;
		ScriptRuntime& runtime;
		std::string path;
		std::string error;
		ScriptProperties properties, defaults, overrides;
		sol::environment environment;
		sol::table self, props;
		ActorHandle handle;
		struct Timer
		{
			double remaining, period;
			sol::protected_function function;
			std::shared_ptr<bool> active;
		};
		std::vector<Timer> timers;
		std::vector<std::uint64_t> sounds;
		std::uint64_t musicRevision = 0;
		std::deque<std::pair<std::string, nlohmann::json>> messages;
		struct KeyState { bool previous = false, current = false; };
		std::map<std::string, KeyState> keys;
		std::filesystem::file_time_type stamp{};
		Clock::time_point nextPoll{};
		Clock::time_point created = Clock::now();
		bool running = false;
		bool calling = false;
		bool ended = false;
		bool endRequested = false;
		std::uint64_t generation = 0;

		Impl(ScriptComponent& owner, std::string script);
		~Impl();
		void Fail(const std::string& message);
		std::filesystem::path FullPath() const;
		sol::object Callback(const char* name);
		void CancelWork();
		void StopAudio();
		std::string AudioPath(const std::string& name) const;
		static void AudioParameters(float volume, float pitch);
		void BindAudio(sol::state& lua);
		template<typename... Args>
		void Invoke(const sol::protected_function& function, Args&&... args);
		template<typename... Args>
		void Call(const char* name, Args&&... args);
		void Tick(float dt);
		void Contact(const char* name, ly::Actor* other) noexcept;
		TimerHandle Schedule(double seconds, sol::protected_function function, bool repeat);
		void BindWorld(sol::state& lua);
		void MakeEnvironment(sol::state& lua);
		bool Load(sol::state& lua, bool start = true);
	};
}
