#include "framework/EngineContext.h"
#include "script/ScriptComponent.h"
#include "script/ScriptRuntime.h"
#include "ScriptComponentInternal.h"
#include "ScriptRuntimeInternal.h"
#include <framework/Actor.h>
#include <framework/ComponentIdentity.h>
#include <framework/World.h>
#include <framework/AudioManager.h>
#include <framework/debug/Log.h>
#include <framework/debug/Profiler.h>
#include <framework/reflection/Reflection.h>
#include <framework/scene/Scene.h>
#include <framework/scene/SceneEntityComponent.h>
#include <framework/scene/PrefabLibrary.h>
#include <sol/sol.hpp>
#include <chrono>
#include <cmath>
#include <fstream>
#include <limits>
#include <cstdlib>
#include <algorithm>
#include <tuple>
#include <deque>
#include <string_view>
#include <vector>

namespace vega::script
{
	using namespace detail;
	ScriptComponent::Impl::Impl(ScriptComponent& owner, std::string script) : component{ owner }, runtime{ ScriptRuntime::Get(owner.GetOwner().GetEngineContext()) }, path{ std::move(script) } {}
	ScriptComponent::Impl::~Impl() { StopAudio(); }
	void ScriptComponent::Impl::Fail(const std::string& message)
	{
		StopAudio();
		error = path + ": " + message;
		running = false;
		const auto marker = message.find(".lua:");
		int line = 0;
		if (marker != std::string::npos) line = std::atoi(message.c_str() + marker + 5);
		ly::debug::Log::Write(ly::debug::LogLevel::Error, ly::debug::LogChannel::Lua,
			{ path.c_str(), "Lua", line }, error);
	}
	std::filesystem::path ScriptComponent::Impl::FullPath() const
	{
		const auto& root = runtime.Root();
		const std::filesystem::path relative{ path };
		if (relative.empty() || relative.is_absolute() || relative.has_root_name() || relative.extension() != ".lua")
			throw std::runtime_error("Expected a relative .lua asset path");
		const auto full = std::filesystem::weakly_canonical(root / relative);
		const auto inside = full.lexically_relative(root);
		if (inside.empty() || *inside.begin() == "..") throw std::runtime_error("Script path escapes asset root");
		return full;
	}
	sol::object ScriptComponent::Impl::Callback(const char* name)
	{
		const auto callback = self.raw_get<sol::object>(name);
		return callback != sol::nil ? callback : environment.raw_get<sol::object>(name);
	}
	void ScriptComponent::Impl::CancelWork()
	{
		StopAudio();
		++generation;
		for (auto& timer : timers) *timer.active = false;
		timers.clear();
		messages.clear();
	}
	void ScriptComponent::Impl::StopAudio()
	{
		if (component.GetOwner().GetEngineContext().HasAudio())
		{
			auto& audio = component.GetOwner().GetEngineContext().Audio();
			for (const auto id : sounds) audio.StopSound(id);
			if (musicRevision && audio.MusicRevision() == musicRevision) audio.StopMusic();
		}
		sounds.clear(); musicRevision = 0;
	}
	template<typename... Args>
	void ScriptComponent::Impl::Invoke(const sol::protected_function& function, Args&&... args)
	{
		calling = true;
		{
			const InstructionScope budget{ function.lua_state() };
			const sol::protected_function_result result = function(self, std::forward<Args>(args)...);
			calling = false;
			if (!result.valid()) { const sol::error failure = result; Fail(failure.what()); }
		}
		if (endRequested)
		{
			endRequested = false;
			Call("on_destroy");
			running = false;
		}
	}
	template<typename... Args>
	void ScriptComponent::Impl::Call(const char* name, Args&&... args)
	{
		if (!running) return;
		const sol::object callback = Callback(name);
		if (callback == sol::nil) return;
		if (callback.get_type() != sol::type::function) { Fail(std::string{ name } + " must be a function"); return; }
		const sol::protected_function function = callback.as<sol::protected_function>();
		// Teardown itself may request destruction during a manual reload.
		if (std::string_view{ name } == "on_destroy")
		{
			calling = true;
			const InstructionScope budget{ function.lua_state() };
			const sol::protected_function_result result = function(self, std::forward<Args>(args)...);
			calling = false;
			if (!result.valid()) { const sol::error failure = result; Fail(failure.what()); }
			if (endRequested) { endRequested = false; running = false; }
			return;
		}
		Invoke(function, std::forward<Args>(args)...);
	}
	void ScriptComponent::Impl::Tick(float dt)
	{
		if (!running || ended) return;
		for (auto& entry : keys)
		{
			entry.second.previous = entry.second.current;
			entry.second.current = KeyDown(handle, entry.first);
		}
		const auto version = generation;
		// Process only work present at entry: callbacks may append more work.
		const auto messageCount = messages.size();
		for (std::size_t index = 0; index < messageCount && running && !ended && generation == version; ++index)
		{
			auto message = std::move(messages.front());
			messages.pop_front();
			Call("on_message", message.first, MessageObject(sol::state_view{ self.lua_state() }, message.second));
		}
		const auto count = timers.size();
		for (std::size_t index = 0; index < count && running && !ended && generation == version; ++index)
		{
			auto& timer = timers[index];
			if (!*timer.active) continue;
			timer.remaining -= std::max(0.f, dt);
			if (timer.remaining > 0.) continue;
			// Copy before invoking: scheduling or destroy can invalidate the vector.
			const auto function = timer.function;
			if (timer.period > 0.) timer.remaining = timer.period + std::fmod(timer.remaining, timer.period);
			else *timer.active = false;
			Invoke(function);
		}
		if (generation != version || ended) return;
		timers.erase(std::remove_if(timers.begin(), timers.end(), [](const Timer& timer) { return !*timer.active; }), timers.end());
		if (!component.GetOwner().GetIsPendingDestroy()) Call("on_update", dt);
	}
	void ScriptComponent::Impl::Contact(const char* name, ly::Actor* other) noexcept
	{
		if (!other || ended || !running) return;
		try { Call(name, ActorHandle{ other->GetWeakActor() }); }
		catch (const std::exception& exception) { calling = false; Fail(exception.what()); }
		catch (...) { calling = false; Fail("Unknown collision callback error"); }
	}
	bool ScriptComponent::Impl::Load(sol::state& lua, bool start)
	{
		if (ended) return false;
		nextPoll = Clock::now() + std::chrono::milliseconds(500);
		const auto full = FullPath();
		std::error_code ec;
		stamp = std::filesystem::last_write_time(full, ec);
		const bool wasRunning = running;
		Call("on_destroy");
		CancelWork();
		// A failing teardown suspends the component just like a failing update.
		if (wasRunning && !error.empty()) return false;
		running = false;
		if (ended) return false;
		error.clear();
		MakeEnvironment(lua);
		std::ifstream input{ full, std::ios::binary };
		if (!input) { Fail("Cannot open script"); return false; }
		const std::string source{ std::istreambuf_iterator<char>{ input }, std::istreambuf_iterator<char>{} };
		// Text-only chunks prevent bytecode from bypassing Lua's source validation.
		const sol::load_result loaded = lua.load(source, "@" + full.generic_string(), sol::load_mode::text);
		if (!loaded.valid()) { const sol::error failure = loaded; Fail(failure.what()); return false; }
		sol::protected_function chunk = loaded;
		sol::set_environment(environment, chunk);
		const sol::protected_function_result result = [&]()
		{
			const InstructionScope budget{ lua.lua_state() };
			return chunk();
		}();
		if (!result.valid()) { const sol::error failure = result; Fail(failure.what()); return false; }
		if (result.return_count() > 0 && result.get_type() == sol::type::table)
		{
			self = result.get<sol::table>();
			self["actor"] = handle;
			self["props"] = props;
		}
		defaults.clear();
		const sol::object declared = Callback("properties");
		if (declared.get_type() == sol::type::table)
		{
			for (const auto& entry : declared.as<sol::table>())
			{
				if (entry.first.get_type() != sol::type::string) continue;
				if (auto value = FromLua(entry.second)) defaults.emplace(entry.first.as<std::string>(), *value);
			}
		}
		properties = defaults;
		for (auto& entry : properties)
		{
			const auto override = overrides.find(entry.first);
			if (override != overrides.end() && override->second.index() == entry.second.index()) entry.second = override->second;
			props[entry.first] = ToLua(lua, entry.second);
		}
		running = start;
		if (start)
		{
			if (Callback("on_start") != sol::nil) Call("on_start");
			else Call("on_create");
		}
		return error.empty();
	}
	ScriptComponent::ScriptComponent(ly::Actor& owner, std::string path)
		: ActorComponent{ owner }, mImpl{ std::make_unique<Impl>(*this, std::move(path)) }
	{
		// The context owns the interpreter and outlives its components.
		mCanEverTick = true;
	}
	ScriptComponent::~ScriptComponent() = default;
	const std::string& ScriptComponent::ScriptPath() const { return mImpl->path; }
	const std::string& ScriptComponent::LastError() const { return mImpl->error; }
	const ScriptProperties& ScriptComponent::Properties() const { return mImpl->properties; }
	const ScriptProperties& ScriptComponent::Overrides() const { return mImpl->overrides; }
	bool ScriptComponent::Prepare() noexcept
	{
		if (HasBegunPlay()) return false;
		try { return mImpl->Load(mImpl->runtime.mImpl->lua, false); }
		catch (const std::exception& error) { mImpl->Fail(error.what()); return false; }
		catch (...) { mImpl->Fail("Unknown script preparation error"); return false; }
	}
	bool ScriptComponent::SetOverrides(const ScriptProperties& values) noexcept
	{
		try
		{
			for (const auto& entry : values)
			{
				const auto& value = entry.second;
				if (!std::holds_alternative<float>(value) && !std::holds_alternative<bool>(value) &&
					!std::holds_alternative<std::string>(value) && !std::holds_alternative<sf::Vector2f>(value)) return false;
				if (const auto* number = std::get_if<float>(&value); number && !std::isfinite(*number)) return false;
				if (const auto* vector = std::get_if<sf::Vector2f>(&value); vector && (!std::isfinite(vector->x) || !std::isfinite(vector->y))) return false;
				if (HasBegunPlay())
				{
					const auto declared = mImpl->defaults.find(entry.first);
					if (declared == mImpl->defaults.end() || declared->second.index() != value.index()) return false;
				}
			}
			mImpl->overrides = values;
			if (HasBegunPlay())
			{
				mImpl->properties = mImpl->defaults;
				for (auto& entry : mImpl->properties)
				{
					const auto override = values.find(entry.first);
					if (override != values.end()) entry.second = override->second;
					mImpl->props[entry.first] = ToLua(mImpl->runtime.mImpl->lua, entry.second);
				}
			}
			return true;
		}
		catch (...) { return false; }
	}
	bool ScriptComponent::Reload() noexcept
	{
		try { return mImpl->Load(mImpl->runtime.mImpl->lua); }
		catch (const std::exception& error) { mImpl->calling = false; mImpl->Fail(error.what()); return false; }
		catch (...) { mImpl->calling = false; mImpl->Fail("Unknown script error"); return false; }
	}
	void ScriptComponent::BeginPlay() { Reload(); }
	void ScriptComponent::TickComponent(float deltaTime)
	{
		LY_PROFILE_SCOPE("Lua Script Tick");
		try
		{
			if (mImpl->ended) return;
			if (mImpl->runtime.HotReload() && Clock::now() >= mImpl->nextPoll)
			{
				mImpl->nextPoll = Clock::now() + std::chrono::milliseconds(500);
				std::error_code ec;
				const auto stamp = std::filesystem::last_write_time(mImpl->FullPath(), ec);
				if (stamp != mImpl->stamp) Reload();
			}
			if (!GetOwner().GetIsPendingDestroy() && !mImpl->ended) mImpl->Tick(deltaTime);
		}
		catch (const std::exception& error) { mImpl->calling = false; mImpl->Fail(error.what()); }
		catch (...) { mImpl->calling = false; mImpl->Fail("Unknown script error"); }
	}
	void ScriptComponent::EndPlay()
	{
		if (mImpl->ended) return;
		mImpl->ended = true;
		mImpl->CancelWork();
		// Actor marks pending-destroy after component teardown; suppress destroy() reentry during that gap.
		*mImpl->handle.destroying = true;
		// destroy() inside a callback must finish that callback before on_destroy, without recursive Lua entry.
		if (mImpl->calling) { mImpl->endRequested = true; return; }
		try { mImpl->Call("on_destroy"); }
		catch (const std::exception& error) { mImpl->Fail(error.what()); }
		catch (...) { mImpl->Fail("Unknown script error"); }
		mImpl->running = false;
	}
	void ScriptComponent::OnOverlapBegin(ly::Actor* other) { mImpl->Contact("on_overlap_begin", other); }
	void ScriptComponent::OnOverlapEnd(ly::Actor* other) { mImpl->Contact("on_overlap_end", other); }
	void ScriptComponent::OnHit(ly::Actor* other) { mImpl->Contact("on_hit", other); }
	bool ScriptComponent::QueueMessage(const std::string& name, const nlohmann::json& data)
	{
		if (!mImpl->running || mImpl->ended || mImpl->messages.size() >= 128 || name.empty() || name.size() > 256) return false;
		mImpl->messages.emplace_back(name, data);
		return true;
	}
	bool ScriptComponent::SetProperty(const std::string& name, const Value& value) noexcept
	{
		try
		{
			const auto entry = mImpl->properties.find(name);
			if (entry == mImpl->properties.end() || entry->second.index() != value.index()) return false;
			if (const auto* number = std::get_if<float>(&value); number && !std::isfinite(*number)) return false;
			if (const auto* vector = std::get_if<sf::Vector2f>(&value); vector && (!std::isfinite(vector->x) || !std::isfinite(vector->y))) return false;
			mImpl->props[name] = ToLua(mImpl->runtime.mImpl->lua, value);
			entry->second = value;
			mImpl->overrides[name] = value;
			return true;
		}
		catch (const std::exception& error) { mImpl->Fail(error.what()); return false; }
		catch (...) { mImpl->Fail("Unknown script property error"); return false; }
	}
	bool ScriptComponent::ClearOverride(const std::string& name) noexcept
	{
		const auto entry = mImpl->defaults.find(name);
		if (entry == mImpl->defaults.end() || !SetProperty(name, entry->second)) return false;
		mImpl->overrides.erase(name);
		return true;
	}
	VEGA_REFLECT(ScriptComponent, ly::ActorComponent)
	{
		VEGA_REMOVABLE();
		VEGA_READONLY("Script", ScriptPath).Asset(ly::reflect::AssetKind::Script);
		VEGA_READONLY("Last Error", LastError);
	}

	void RegisterScriptTypes() { RegisterScriptTypes(ly::EngineContext::Current()); }

	void RegisterScriptTypes(ly::EngineContext& context)
	{
		ly::reflect::RegisterEngineTypes(context.Types());
		VEGA_REGISTER_IN(ScriptComponent, context.Types());
		ly::SceneSerializer::RegisterComponentSerializer("ScriptComponent", {
			"scripts",
			[](const ly::ActorComponent& base)
			{
				const auto& component = dynamic_cast<const ScriptComponent&>(base);
				nlohmann::json properties = nlohmann::json::object();
				for (const auto& entry : component.Overrides())
					properties[entry.first] = std::visit([](const auto& value) -> nlohmann::json
					{
						using T = std::decay_t<decltype(value)>;
						if constexpr (std::is_same_v<T, sf::Vector2f>) return nlohmann::json::array({ value.x, value.y });
						else if constexpr (std::is_same_v<T, sf::Color>) return nullptr;
						else if constexpr (std::is_same_v<T, ly::ActorRef> || std::is_same_v<T, ly::reflect::PropertyList>) return nullptr;
						else return value;
					}, entry.second);
				return nlohmann::json{ { "path", component.ScriptPath() }, { "properties", std::move(properties) } };
			},
			[](ly::Actor& actor, const nlohmann::json& data, ly::SceneLoadResult& result)
			{
				const auto path = data.at("path").get<std::string>();
				const auto& properties = data.at("properties");
				if (!properties.is_object()) throw std::runtime_error("Script properties must be an object");
				auto& component = actor.AddComponent<ScriptComponent>(path);
				if (!component.Prepare())
				{
					result.warnings.push_back(component.LastError());
					return;
				}
				ScriptProperties overrides;
				for (const auto& entry : properties.items())
				{
					const auto& value = entry.value();
					if (value.is_boolean()) overrides[entry.key()] = value.get<bool>();
					else if (value.is_string()) overrides[entry.key()] = value.get<std::string>();
					else if (value.is_number() && std::isfinite(value.get<float>())) overrides[entry.key()] = value.get<float>();
					else if (value.is_array() && value.size() == 2 && value[0].is_number() && value[1].is_number() &&
						std::isfinite(value[0].get<float>()) && std::isfinite(value[1].get<float>()))
						overrides[entry.key()] = sf::Vector2f{ value[0].get<float>(), value[1].get<float>() };
					else result.warnings.push_back(path + "." + entry.key() + ": unsupported script override");
				}
				for (const auto& entry : overrides)
					if (!component.SetProperty(entry.first, entry.second))
						result.warnings.push_back(path + "." + entry.first + ": unknown property or incompatible override");
			}
		}, context);
	}
}
