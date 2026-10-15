#include "script/ScriptComponent.h"
#include "script/ScriptRuntime.h"
#include <framework/Actor.h>
#include <framework/World.h>
#include <framework/debug/Log.h>
#include <sol/sol.hpp>
#include <chrono>
#include <cmath>
#include <fstream>
#include <limits>

namespace vega::script
{
	namespace
	{
		using Value = ly::reflect::PropertyValue;
		using Clock = std::chrono::steady_clock;
		struct ActorHandle
		{
			std::weak_ptr<ly::Actor> actor;
			std::shared_ptr<bool> destroying = std::make_shared<bool>(false);
			std::shared_ptr<ly::Actor> Lock() const
			{
				auto result = actor.lock();
				return result && !result->GetIsPendingDestroy() ? result : nullptr;
			}
		};
		struct ComponentHandle
		{
			ActorHandle owner;
			std::size_t index;
			ly::ActorComponent* Resolve(const std::shared_ptr<ly::Actor>& actor) const
			{
				ly::ActorComponent* result = nullptr;
				std::size_t current = 0;
				if (actor) actor->ForEachComponent([&](ly::ActorComponent& component)
				{
					if (current++ == index) result = &component;
				});
				return result;
			}
		};
		sol::object ToLua(sol::state_view lua, const Value& value)
		{
			return std::visit([&](const auto& item) -> sol::object
			{
				using T = std::decay_t<decltype(item)>;
				if constexpr (std::is_same_v<T, sf::Vector2f>)
					return sol::make_object(lua, lua.create_table_with("x", item.x, "y", item.y));
				else if constexpr (std::is_same_v<T, sf::Color>) return sol::make_object(lua, sol::nil);
				else return sol::make_object(lua, item);
			}, value);
		}
		std::optional<Value> FromLua(const sol::object& object)
		{
			if (object.get_type() == sol::type::boolean) return Value{ object.as<bool>() };
			if (object.get_type() == sol::type::string) return Value{ object.as<std::string>() };
			if (object.get_type() == sol::type::number)
			{
				const float value = object.as<float>();
				if (std::isfinite(value)) return Value{ value };
			}
			if (object.get_type() == sol::type::table)
			{
				const sol::table table = object.as<sol::table>();
				const sol::object x = table.raw_get<sol::object>("x"), y = table.raw_get<sol::object>("y");
				if (x.get_type() == sol::type::number && y.get_type() == sol::type::number)
				{
					const sf::Vector2f value{ x.as<float>(), y.as<float>() };
					if (std::isfinite(value.x) && std::isfinite(value.y)) return Value{ value };
				}
			}
			return {};
		}
		sol::object ReadProperty(sol::this_state state, const void* object, const ly::reflect::TypeInfo* type, const std::string& name)
		{
			const auto* property = type ? type->FindProperty(name) : nullptr;
			return object && property ? ToLua(sol::state_view{ state }, property->getter(object)) : sol::make_object(state, sol::nil);
		}
		bool WriteProperty(void* object, const ly::reflect::TypeInfo* type, const std::string& name, const sol::object& input)
		{
			const auto* property = type ? type->FindProperty(name) : nullptr;
			auto value = FromLua(input);
			if (!object || !property || !value) return false;
			if (property->kind == ly::reflect::PropertyKind::Int || property->kind == ly::reflect::PropertyKind::Enum)
			{
				const auto* number = std::get_if<float>(&*value);
				if (!number || std::trunc(*number) != *number || static_cast<double>(*number) < std::numeric_limits<int>::min() ||
					static_cast<double>(*number) > std::numeric_limits<int>::max()) return false;
				value = static_cast<int>(*number);
			}
			return property->SetValue(object, *value);
		}
		bool KeyDown(const ActorHandle& handle, const std::string& name)
		{
			const auto actor = handle.Lock();
			if (!actor || !actor->GetWorld() || !actor->GetWorld()->HasInputFocus()) return false;
			using Key = sf::Keyboard::Key;
			Key key = Key::Unknown;
			if (name.size() == 1 && name[0] >= 'A' && name[0] <= 'Z') key = static_cast<Key>(static_cast<int>(Key::A) + name[0] - 'A');
			else if (name == "Space") key = Key::Space;
			else if (name == "Left") key = Key::Left;
			else if (name == "Right") key = Key::Right;
			else if (name == "Up") key = Key::Up;
			else if (name == "Down") key = Key::Down;
			return key != Key::Unknown && sf::Keyboard::isKeyPressed(key);
		}
		void Bind(sol::state& lua)
		{
			lua.new_usertype<ComponentHandle>("ComponentHandle", sol::no_constructor,
				"get", [](const ComponentHandle& handle, const std::string& name, sol::this_state state)
				{
					const auto actor = handle.owner.Lock();
					const auto* component = handle.Resolve(actor);
					return ReadProperty(state, component, component ? ly::reflect::TypeRegistry::Get().FindMostDerived(*component) : nullptr, name);
				},
				"set", [](const ComponentHandle& handle, const std::string& name, sol::object value)
				{
					const auto actor = handle.owner.Lock();
					auto* component = handle.Resolve(actor);
					return WriteProperty(component, component ? ly::reflect::TypeRegistry::Get().FindMostDerived(*component) : nullptr, name, value);
				});
			lua.new_usertype<ActorHandle>("ActorHandle", sol::no_constructor,
				"is_valid", [](const ActorHandle& h) { return static_cast<bool>(h.Lock()); },
				"id", [](const ActorHandle& h) { const auto a = h.Lock(); return a ? a->GetUniqueID() : 0u; },
				"type", [](const ActorHandle& h)
				{
					const auto a = h.Lock();
					const auto* type = a ? ly::reflect::TypeRegistry::Get().FindMostDerived(*a) : nullptr;
					return type ? type->name : std::string{};
				},
				"location", [](const ActorHandle& h, sol::this_state s) { const auto a = h.Lock(); return ToLua(sol::state_view{ s }, a ? a->GetActorLocation() : sf::Vector2f{}); },
				"set_location", [](const ActorHandle& h, float x, float y) { if (auto a = h.Lock(); a && std::isfinite(x) && std::isfinite(y)) a->SetActorLocation({ x, y }); },
				"rotation", [](const ActorHandle& h) { const auto a = h.Lock(); return a ? a->GetActorRotation() : 0.f; },
				"set_rotation", [](const ActorHandle& h, float degrees) { if (auto a = h.Lock(); a && std::isfinite(degrees)) a->SetActorRotation(degrees); },
				"velocity", [](const ActorHandle& h, sol::this_state s) { const auto a = h.Lock(); return ToLua(sol::state_view{ s }, a ? a->GetVelocity() : sf::Vector2f{}); },
				"set_velocity", [](const ActorHandle& h, float x, float y) { if (auto a = h.Lock(); a && std::isfinite(x) && std::isfinite(y)) a->SetVelocity({ x, y }); },
				"destroy", [](const ActorHandle& h)
				{
					if (auto a = h.Lock(); a && !*h.destroying)
					{
						*h.destroying = true;
						a->Destroy();
					}
				},
				"get", [](const ActorHandle& h, const std::string& name, sol::this_state s)
				{
					const auto a = h.Lock();
					return ReadProperty(s, a.get(), a ? ly::reflect::TypeRegistry::Get().FindMostDerived(*a) : nullptr, name);
				},
				"set", [](const ActorHandle& h, const std::string& name, sol::object value)
				{
					const auto a = h.Lock();
					return WriteProperty(a.get(), a ? ly::reflect::TypeRegistry::Get().FindMostDerived(*a) : nullptr, name, value);
				},
				"component", [](const ActorHandle& h, const std::string& name, sol::this_state s)
				{
					const auto a = h.Lock();
					sol::object result = sol::make_object(s, sol::nil);
					std::size_t index = 0;
					if (a) a->ForEachComponent([&](ly::ActorComponent& component)
					{
						for (auto* type = ly::reflect::TypeRegistry::Get().FindMostDerived(component); type; type = type->base)
							if (result == sol::nil && type->name == name) result = sol::make_object(s, ComponentHandle{ h, index });
						++index;
					});
					return result;
				});
		}
	}

	struct ScriptRuntime::Impl
	{
		sol::state lua;
		std::filesystem::path root;
#ifdef VEGA_SCRIPT_RELEASE
		bool hotReload = false;
#else
		bool hotReload = true;
#endif
		Impl()
		{
			lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);
			Bind(lua);
		}
	};
	ScriptRuntime::ScriptRuntime() : mImpl{ std::make_unique<Impl>() } {}
	ScriptRuntime::~ScriptRuntime() = default;
	ScriptRuntime& ScriptRuntime::Get() { static ScriptRuntime runtime; return runtime; }
	void ScriptRuntime::SetHotReload(bool enabled) { mImpl->hotReload = enabled; }
	bool ScriptRuntime::HotReload() const { return mImpl->hotReload; }
	const std::filesystem::path& ScriptRuntime::Root() const { return mImpl->root; }
	void SetScriptRoot(const std::filesystem::path& root) { ScriptRuntime::Get().mImpl->root = std::filesystem::weakly_canonical(root); }

	struct ScriptComponent::Impl
	{
		ScriptComponent& component;
		std::string path;
		std::string error;
		ScriptProperties properties, defaults, overrides;
		sol::environment environment;
		sol::table self, props;
		ActorHandle handle;
		std::filesystem::file_time_type stamp{};
		Clock::time_point nextPoll{};
		Clock::time_point created = Clock::now();
		bool running = false;
		bool calling = false;
		bool ended = false;
		bool endRequested = false;
		Impl(ScriptComponent& owner, std::string script) : component{ owner }, path{ std::move(script) } {}
		void Fail(const std::string& message)
		{
			error = path + ": " + message;
			running = false;
			LY_CORE_ERROR("Lua %s", error.c_str());
		}
		std::filesystem::path FullPath() const
		{
			const auto& root = ScriptRuntime::Get().Root();
			const std::filesystem::path relative{ path };
			if (relative.empty() || relative.is_absolute() || relative.has_root_name() || relative.extension() != ".lua")
				throw std::runtime_error("Expected a relative .lua asset path");
			const auto full = std::filesystem::weakly_canonical(root / relative);
			const auto inside = full.lexically_relative(root);
			if (inside.empty() || *inside.begin() == "..") throw std::runtime_error("Script path escapes asset root");
			return full;
		}
		void Call(const char* name, std::optional<float> dt = {})
		{
			if (!running) return;
			const sol::object callback = environment.raw_get<sol::object>(name);
			if (callback == sol::nil) return;
			if (callback.get_type() != sol::type::function) { Fail(std::string{ name } + " must be a function"); return; }
			calling = true;
			const sol::protected_function function = callback.as<sol::protected_function>();
			const sol::protected_function_result result = dt ? function(self, *dt) : function(self);
			calling = false;
			if (!result.valid()) { const sol::error failure = result; Fail(failure.what()); }
			if (endRequested)
			{
				endRequested = false;
				if (std::string{ name } != "on_destroy") Call("on_destroy");
				running = false;
			}
		}
		void MakeEnvironment(sol::state& lua)
		{
			// No fallback to interpreter globals: even _G and mutable library tables are instance-local.
			environment = sol::environment{ lua, sol::create };
			for (const char* name : { "assert", "error", "ipairs", "next", "pairs", "pcall", "select", "tonumber", "tostring", "type", "xpcall", "rawequal", "rawget", "rawlen", "rawset", "_VERSION" })
				environment[name] = lua[name];
			for (const char* name : { "math", "string", "table" })
			{
				sol::table copy = lua.create_table();
				const sol::table original = lua[name];
				for (const auto& entry : original) copy[entry.first] = entry.second;
				environment[name] = copy;
			}
			environment["_G"] = environment;
			environment["log"] = [](const std::string& message) { LY_CORE_INFO("Lua: %s", message.c_str()); };
			const auto start = created;
			environment["time"] = [start]() { return std::chrono::duration<double>(Clock::now() - start).count(); };
			handle = ActorHandle{ component.GetOwner().GetWeakActor() };
			const ActorHandle inputHandle = handle;
			sol::table input = lua.create_table();
			input["key_down"] = [inputHandle](const std::string& name) { return KeyDown(inputHandle, name); };
			environment["input"] = input;
			props = lua.create_table();
			self = lua.create_table_with("actor", handle, "props", props);
		}
		bool Load(sol::state& lua)
		{
			if (ended) return false;
			nextPoll = Clock::now() + std::chrono::milliseconds(500);
			const auto full = FullPath();
			std::error_code ec;
			stamp = std::filesystem::last_write_time(full, ec);
			const bool wasRunning = running;
			Call("on_destroy");
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
			const sol::protected_function_result result = chunk();
			if (!result.valid()) { const sol::error failure = result; Fail(failure.what()); return false; }
			defaults.clear();
			const sol::object declared = environment.raw_get<sol::object>("properties");
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
			running = true;
			Call("on_create");
			return error.empty();
		}
	};

	ScriptComponent::ScriptComponent(ly::Actor& owner, std::string path)
		: ActorComponent{ owner }, mImpl{ std::make_unique<Impl>(*this, std::move(path)) }
	{
		// Construct the singleton before components so it outlives their Lua references.
		ScriptRuntime::Get();
		mCanEverTick = true;
	}
	ScriptComponent::~ScriptComponent() = default;
	const std::string& ScriptComponent::ScriptPath() const { return mImpl->path; }
	const std::string& ScriptComponent::LastError() const { return mImpl->error; }
	const ScriptProperties& ScriptComponent::Properties() const { return mImpl->properties; }
	const ScriptProperties& ScriptComponent::Overrides() const { return mImpl->overrides; }
	bool ScriptComponent::Reload() noexcept
	{
		try { return mImpl->Load(ScriptRuntime::Get().mImpl->lua); }
		catch (const std::exception& error) { mImpl->calling = false; mImpl->Fail(error.what()); return false; }
		catch (...) { mImpl->calling = false; mImpl->Fail("Unknown script error"); return false; }
	}
	void ScriptComponent::BeginPlay() { Reload(); }
	void ScriptComponent::TickComponent(float deltaTime)
	{
		try
		{
			if (mImpl->ended) return;
			if (ScriptRuntime::Get().HotReload() && Clock::now() >= mImpl->nextPoll)
			{
				mImpl->nextPoll = Clock::now() + std::chrono::milliseconds(500);
				std::error_code ec;
				const auto stamp = std::filesystem::last_write_time(mImpl->FullPath(), ec);
				if (stamp != mImpl->stamp) Reload();
			}
			if (!GetOwner().GetIsPendingDestroy() && !mImpl->ended) mImpl->Call("on_update", deltaTime);
		}
		catch (const std::exception& error) { mImpl->calling = false; mImpl->Fail(error.what()); }
		catch (...) { mImpl->calling = false; mImpl->Fail("Unknown script error"); }
	}
	void ScriptComponent::EndPlay()
	{
		if (mImpl->ended) return;
		mImpl->ended = true;
		// Actor marks pending-destroy after component teardown; suppress destroy() reentry during that gap.
		*mImpl->handle.destroying = true;
		// destroy() inside a callback must finish that callback before on_destroy, without recursive Lua entry.
		if (mImpl->calling) { mImpl->endRequested = true; return; }
		try { mImpl->Call("on_destroy"); }
		catch (const std::exception& error) { mImpl->Fail(error.what()); }
		catch (...) { mImpl->Fail("Unknown script error"); }
		mImpl->running = false;
	}
	bool ScriptComponent::SetProperty(const std::string& name, const Value& value) noexcept
	{
		try
		{
			const auto entry = mImpl->properties.find(name);
			if (entry == mImpl->properties.end() || entry->second.index() != value.index()) return false;
			if (const auto* number = std::get_if<float>(&value); number && !std::isfinite(*number)) return false;
			if (const auto* vector = std::get_if<sf::Vector2f>(&value); vector && (!std::isfinite(vector->x) || !std::isfinite(vector->y))) return false;
			mImpl->props[name] = ToLua(ScriptRuntime::Get().mImpl->lua, value);
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
	void RegisterScriptTypes()
	{
		ly::reflect::RegisterEngineTypes();
		ly::reflect::Class<ScriptComponent, ly::ActorComponent>("ScriptComponent")
			.Property("Script", [](const ScriptComponent& c) { return c.ScriptPath(); })
			.Property("Last Error", [](const ScriptComponent& c) { return c.LastError(); });
	}
}
