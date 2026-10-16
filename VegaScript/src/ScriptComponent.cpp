#include "script/ScriptComponent.h"
#include "script/ScriptRuntime.h"
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
	namespace
	{
		using Value = ly::reflect::PropertyValue;
		using Clock = std::chrono::steady_clock;
		int LuaPrint(lua_State* state)
		{
			std::string message;
			const int count = lua_gettop(state);
			for (int index = 1; index <= count; ++index)
			{
				std::size_t length = 0;
				const char* text = luaL_tolstring(state, index, &length);
				if (index > 1) message += '\t';
				message.append(text, length);
				lua_pop(state, 1);
			}
			lua_Debug source{};
			if (lua_getstack(state, 1, &source)) lua_getinfo(state, "Sl", &source);
			ly::debug::Log::Write(ly::debug::LogLevel::Info, ly::debug::LogChannel::Lua,
				{ source.source && source.source[0] == '@' ? source.source + 1 : "", "print", source.currentline }, std::move(message));
			return 0;
		}
		struct RuntimeLimits
		{
			std::size_t memoryLimit = 256u * 1024u * 1024u;
			std::size_t memoryUsed = 0;
			std::uint64_t instructionBudget = 10000000;
			std::uint64_t remaining = 0;
			int hookCount = 1000;
			unsigned int depth = 0;
		};
		void* Allocate(void* user, void* pointer, std::size_t oldSize, std::size_t newSize)
		{
			auto& limits = *static_cast<RuntimeLimits*>(user);
			// Lua uses oldSize as a type tag for new objects, not an allocation size.
			if (!pointer) oldSize = 0;
			if (!newSize)
			{
				std::free(pointer);
				limits.memoryUsed -= oldSize;
				return nullptr;
			}
			if (newSize > oldSize && (limits.memoryUsed >= limits.memoryLimit ||
				newSize - oldSize > limits.memoryLimit - limits.memoryUsed)) return nullptr;
			void* result = std::realloc(pointer, newSize);
			if (result) limits.memoryUsed = limits.memoryUsed - oldSize + newSize;
			return result;
		}
		void InstructionHook(lua_State* state, lua_Debug*)
		{
			void* user = nullptr;
			lua_getallocf(state, &user);
			auto& limits = *static_cast<RuntimeLimits*>(user);
			if (limits.remaining <= static_cast<std::uint64_t>(limits.hookCount))
			{
				limits.remaining = 0;
				luaL_error(state, "script exceeded its instruction budget");
			}
			limits.remaining -= limits.hookCount;
		}
		struct InstructionScope
		{
			lua_State* state;
			explicit InstructionScope(lua_State* lua) : state{ lua }
			{
				void* user = nullptr;
				lua_getallocf(state, &user);
				auto& limits = *static_cast<RuntimeLimits*>(user);
				if (limits.depth++ != 0) return;
				limits.remaining = limits.instructionBudget;
				limits.hookCount = static_cast<int>(std::min<std::uint64_t>(1000, limits.instructionBudget));
				lua_sethook(state, InstructionHook, LUA_MASKCOUNT, limits.hookCount);
			}
			~InstructionScope()
			{
				void* user = nullptr;
				lua_getallocf(state, &user);
				if (--static_cast<RuntimeLimits*>(user)->depth == 0) lua_sethook(state, nullptr, 0, 0);
			}
		};
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
			ly::ComponentIdentity identity;
			ly::ActorComponent* Resolve(const std::shared_ptr<ly::Actor>& actor) const
			{
				return actor ? ly::FindComponentByTypeOccurrence(*actor, identity) : nullptr;
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
			void Stop() const { if (ly::AudioManager::IsInitialized()) ly::AudioManager::GetAudioManager().StopSound(id); }
			bool Playing() const { return ly::AudioManager::IsInitialized() && ly::AudioManager::GetAudioManager().IsSoundPlaying(id); }
		};
		bool Matches(const ly::Actor& actor, const std::string& name)
		{
			if (name.empty()) return true;
			for (auto* type = ly::reflect::TypeRegistry::Get().FindMostDerived(actor); type; type = type->base)
				if (type->name == name) return true;
			return false;
		}
		nlohmann::json MessageData(const sol::object& object, unsigned int& entries, unsigned int depth = 0)
		{
			if (++entries > 256 || depth > 8) throw std::runtime_error("Message data exceeds 256 values or 8 levels");
			switch (object.get_type())
			{
			case sol::type::nil: return nullptr;
			case sol::type::boolean: return object.as<bool>();
			case sol::type::number:
			{
				const double number = object.as<double>();
				if (!std::isfinite(number)) throw std::runtime_error("Expected finite message number");
				return number;
			}
			case sol::type::string:
			{
				const auto value = object.as<std::string>();
				if (value.size() > 4096) throw std::runtime_error("Message string exceeds 4096 bytes");
				return value;
			}
			case sol::type::table:
			{
				nlohmann::json data = nlohmann::json::object();
				for (const auto& entry : object.as<sol::table>())
				{
					if (entry.first.get_type() != sol::type::string) throw std::runtime_error("Message tables require string keys");
					const auto key = entry.first.as<std::string>();
					if (key.size() > 256) throw std::runtime_error("Message key exceeds 256 bytes");
					data[key] = MessageData(entry.second, entries, depth + 1);
				}
				return data;
			}
			default: throw std::runtime_error("Message data supports nil, booleans, numbers, strings and string-keyed tables");
			}
		}
		sol::object MessageObject(sol::state_view lua, const nlohmann::json& data)
		{
			if (data.is_null()) return sol::make_object(lua, sol::nil);
			if (data.is_boolean()) return sol::make_object(lua, data.get<bool>());
			if (data.is_number()) return sol::make_object(lua, data.get<double>());
			if (data.is_string()) return sol::make_object(lua, data.get<std::string>());
			sol::table table = lua.create_table();
			for (const auto& entry : data.items()) table[entry.key()] = MessageObject(lua, entry.value());
			return sol::make_object(lua, table);
		}
		sol::object ToLua(sol::state_view lua, const Value& value, ly::World* world = nullptr)
		{
			return std::visit([&](const auto& item) -> sol::object
			{
				using T = std::decay_t<decltype(item)>;
				if constexpr (std::is_same_v<T, sf::Vector2f>)
					return sol::make_object(lua, Vector{ item.x, item.y });
				else if constexpr (std::is_same_v<T, sf::Color>)
				{
					sol::table color = lua.create_table();
					color["r"] = item.r; color["g"] = item.g; color["b"] = item.b; color["a"] = item.a;
					return sol::make_object(lua, color);
				}
				else if constexpr (std::is_same_v<T, ly::ActorRef>)
				{
					const auto actor = world ? item.Resolve(*world) : nullptr;
					return actor ? sol::make_object(lua, ActorHandle{ actor }) : sol::make_object(lua, sol::nil);
				}
				else if constexpr (std::is_same_v<T, ly::reflect::PropertyList>)
				{
					sol::table list = lua.create_table();
					list["n"] = item.size();
					for (std::size_t i = 0; i < item.size(); ++i) list[i + 1] = ToLua(lua, ly::reflect::ToValue(item[i]), world);
					return sol::make_object(lua, list);
				}
				else return sol::make_object(lua, item);
			}, value);
		}
		std::optional<Value> FromLua(const sol::object& object)
		{
			if (object.is<Vector>())
			{
				const auto value = object.as<Vector>();
				if (std::isfinite(value.x) && std::isfinite(value.y)) return Value{ sf::Vector2f{ value.x, value.y } };
			}
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
			return object && property ? ToLua(sol::state_view{ state }, property->getter(object), property->world ? property->world(object) : nullptr) : sol::make_object(state, sol::nil);
		}
		std::optional<Value> FromPropertyLua(const ly::reflect::PropertyInfo& property, const sol::object& input, ly::World* world)
		{
			using ly::reflect::PropertyKind;
			if (property.kind == PropertyKind::ActorRef)
			{
				if (input == sol::nil) return ly::ActorRef{};
				if (!input.is<ActorHandle>()) return {};
				const auto actor = input.as<ActorHandle>().Lock();
				if (!actor || actor->GetWorld() != world) return {};
				return ly::ActorRef{ *actor };
			}
			if (property.kind == PropertyKind::List)
			{
				if (input.get_type() != sol::type::table) return {};
				const auto table = input.as<sol::table>();
				const auto length = table.raw_get<sol::object>("n");
				if (length != sol::nil && (!length.is<int>() || length.as<int>() < 0)) return {};
				const auto size = length == sol::nil ? table.size() : static_cast<std::size_t>(length.as<int>());
				if (size > 4096) return {};
				std::size_t count = 0;
				for (const auto& entry : table)
				{
					if (entry.first.get_type() == sol::type::string && entry.first.as<std::string>() == "n") continue;
					if (!entry.first.is<int>()) return {};
					const int index = entry.first.as<int>();
					if (index < 1 || static_cast<std::size_t>(index) > size) return {};
					++count;
				}
				if (count != size && property.elementKind != PropertyKind::ActorRef) return {};
				ly::reflect::PropertyList list;
				for (std::size_t i = 1; i <= size; ++i)
				{
					auto value = FromPropertyLua(property.Element(), table.raw_get<sol::object>(i), world);
					if (!value) return {};
					list.push_back(ly::reflect::ToScalar(*value));
				}
				return list;
			}
			if (property.kind == PropertyKind::Color)
			{
				if (input.get_type() != sol::type::table) return {};
				const auto table = input.as<sol::table>();
				int channels[4]{};
				const char* names[]{ "r", "g", "b", "a" };
				for (int i = 0; i < 4; ++i)
				{
					const auto channel = table.raw_get<sol::object>(names[i]);
					if (!channel.is<int>()) return {};
					channels[i] = channel.as<int>();
					if (channels[i] < 0 || channels[i] > 255) return {};
				}
				return sf::Color{ static_cast<std::uint8_t>(channels[0]), static_cast<std::uint8_t>(channels[1]), static_cast<std::uint8_t>(channels[2]), static_cast<std::uint8_t>(channels[3]) };
			}
			auto value = FromLua(input);
			if (property.kind == PropertyKind::Int || property.kind == PropertyKind::Enum || property.kind == PropertyKind::Flags)
			{
				if (input.get_type() != sol::type::number) return {};
				const double number = input.as<double>();
				if (!std::isfinite(number) || std::trunc(number) != number || number < std::numeric_limits<int>::min() || number > std::numeric_limits<int>::max()) return {};
				value = static_cast<int>(number);
			}
			return value;
		}
		std::tuple<bool, std::string> WriteProperty(void* object, const ly::reflect::TypeInfo* type, const std::string& name, const sol::object& input)
		{
			const auto* property = type ? type->FindProperty(name) : nullptr;
			auto value = object && property ? FromPropertyLua(*property, input, property->world ? property->world(object) : nullptr) : std::optional<Value>{};
			if (!object || !property || !value) return { false, "Unknown target/property or incompatible value" };
			std::string error;
			const bool ok = property->SetValue(object, *value, &error);
			return { ok, std::move(error) };
		}
		bool KeyDown(const ActorHandle& handle, const std::string& name)
		{
			const auto actor = handle.Lock();
			if (!actor || !actor->GetWorld() || !actor->GetWorld()->HasInputFocus()) return false;
			return ScriptRuntime::Get().ReadKey(name);
		}
		bool DeviceKey(const std::string& name)
		{
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
			lua.new_usertype<SoundHandle>("VegaSoundHandle", sol::no_constructor,
				"stop", &SoundHandle::Stop, "is_playing", &SoundHandle::Playing);
			lua.new_usertype<Vector>("VegaVector", sol::no_constructor,
				"x", &Vector::x, "y", &Vector::y,
				"length", [](Vector v) { return std::hypot(v.x, v.y); },
				"normalize", [](Vector v) { const float n = std::hypot(v.x, v.y); return n > 0.f ? Vector{ v.x / n, v.y / n } : Vector{}; },
				"dot", [](Vector a, Vector b) { return a.x * b.x + a.y * b.y; },
				sol::meta_function::addition, [](Vector a, Vector b) { return Vector{ a.x + b.x, a.y + b.y }; },
				sol::meta_function::subtraction, [](Vector a, Vector b) { return Vector{ a.x - b.x, a.y - b.y }; },
				sol::meta_function::multiplication, sol::overload(
					[](Vector a, float n) { return Vector{ a.x * n, a.y * n }; },
					[](float n, Vector a) { return Vector{ a.x * n, a.y * n }; }));
			lua.new_usertype<TimerHandle>("TimerHandle", sol::no_constructor,
				"cancel", &TimerHandle::Cancel,
				"is_active", [](const TimerHandle& h) { const auto active = h.active.lock(); return active && *active; });
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
				"parent", [](const ActorHandle& h, sol::this_state s)
				{
					const auto actor = h.Lock();
					return actor && actor->GetParent() ? sol::make_object(s, ActorHandle{ actor->GetParent()->GetWeakActor() }) : sol::make_object(s, sol::nil);
				},
				"children", [](const ActorHandle& h, sol::this_state s)
				{
					auto result = sol::state_view{ s }.create_table();
					int index = 1;
					if (const auto actor = h.Lock()) for (auto* child : actor->GetChildren())
						if (!child->GetIsPendingDestroy()) result[index++] = ActorHandle{ child->GetWeakActor() };
					return result;
				},
				"attach_to", [](const ActorHandle& h, const ActorHandle& other, sol::optional<bool> keepWorld)
				{
					const auto actor = h.Lock(), parent = other.Lock();
					return actor && parent && actor->AttachTo(parent.get(), keepWorld.value_or(true));
				},
				"detach", [](const ActorHandle& h) { if (const auto actor = h.Lock()) actor->Detach(); },
				"is_alive", [](const ActorHandle& h) { return static_cast<bool>(h.Lock()); },
				"name", [](const ActorHandle& h)
				{
					const auto actor = h.Lock();
					const auto* identity = actor ? actor->FindComponent<ly::SceneEntityComponent>() : nullptr;
					return identity ? identity->name : std::string{};
				},
				"send", [](const ActorHandle& h, const std::string& name, sol::optional<sol::object> value)
				{
					const auto actor = h.Lock();
					if (!actor || name.empty() || name.size() > 256) return false;
					unsigned int count = 0;
					const auto data = value ? MessageData(*value, count) : nlohmann::json{};
					if (data.dump().size() > 65536) throw std::runtime_error("Message data exceeds 65536 bytes");
					bool sent = false;
					actor->ForEachComponent([&](ly::ActorComponent& component)
					{
						if (auto* script = dynamic_cast<ScriptComponent*>(&component)) sent = script->QueueMessage(name, data) || sent;
					});
					return sent;
				},
				"distance_to", [](const ActorHandle& a, const ActorHandle& b)
				{
					const auto first = a.Lock(), second = b.Lock();
					if (!first || !second) return std::numeric_limits<float>::infinity();
					const auto d = first->GetActorLocation() - second->GetActorLocation();
					return std::hypot(d.x, d.y);
				},
				"forward", [](const ActorHandle& h)
				{
					const auto actor = h.Lock();
					const auto v = actor ? actor->GetActorForwardDirection() : sf::Vector2f{};
					return Vector{ v.x, v.y };
				},
				"look_at", [](const ActorHandle& h, float x, float y)
				{
					if (auto actor = h.Lock(); actor && std::isfinite(x) && std::isfinite(y))
					{
						const auto d = sf::Vector2f{ x, y } - actor->GetActorLocation();
						if (d.x != 0.f || d.y != 0.f) actor->SetActorRotation(std::atan2(d.y, d.x) * 180.f / 3.141592654f + 90.f);
					}
				},
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

					if (a) a->ForEachComponent([&](ly::ActorComponent& component)
					{
						for (auto* type = ly::reflect::TypeRegistry::Get().FindMostDerived(component); type; type = type->base)
							if (result == sol::nil && type->name == name) result = sol::make_object(s, ComponentHandle{ h, *ly::IdentifyComponent(*a, component) });

					});
					return result;
				});
		}
	}

	struct ScriptRuntime::Impl
	{
		// The allocator's accounting must outlive the interpreter, including lua_close.
		RuntimeLimits limits;
		sol::state lua{ sol::default_at_panic, Allocate, &limits };
		std::filesystem::path root;
		PlayerResolver player;
		KeyReader keyReader;
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
	void ScriptRuntime::SetInstructionBudget(std::uint64_t instructions) { mImpl->limits.instructionBudget = std::max<std::uint64_t>(1, instructions); }
	void ScriptRuntime::SetMemoryLimit(std::size_t bytes) { mImpl->limits.memoryLimit = bytes; }
	const std::filesystem::path& ScriptRuntime::Root() const { return mImpl->root; }
	void ScriptRuntime::SetPlayerResolver(PlayerResolver resolver) { mImpl->player = std::move(resolver); }
	std::shared_ptr<ly::Actor> ScriptRuntime::Player(ly::World& world) const
	{
		const auto actor = mImpl->player ? mImpl->player(world).lock() : nullptr;
		return actor && actor->GetWorld() == &world && !actor->GetIsPendingDestroy() ? actor : nullptr;
	}
	void ScriptRuntime::SetKeyReader(KeyReader reader) { mImpl->keyReader = std::move(reader); }
	bool ScriptRuntime::ReadKey(const std::string& key) const { return mImpl->keyReader ? mImpl->keyReader(key) : DeviceKey(key); }
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
		Impl(ScriptComponent& owner, std::string script) : component{ owner }, path{ std::move(script) } {}
		~Impl() { StopAudio(); }
		void Fail(const std::string& message)
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
		sol::object Callback(const char* name)
		{
			const auto callback = self.raw_get<sol::object>(name);
			return callback != sol::nil ? callback : environment.raw_get<sol::object>(name);
		}
		void CancelWork()
		{
			StopAudio();
			++generation;
			for (auto& timer : timers) *timer.active = false;
			timers.clear();
			messages.clear();
		}
		void StopAudio()
		{
			if (ly::AudioManager::IsInitialized())
			{
				auto& audio = ly::AudioManager::GetAudioManager();
				for (const auto id : sounds) audio.StopSound(id);
				if (musicRevision && audio.MusicRevision() == musicRevision) audio.StopMusic();
			}
			sounds.clear(); musicRevision = 0;
		}
		std::string AudioPath(const std::string& name) const
		{
			if (!running || ended || component.GetOwner().GetIsPendingDestroy()) throw std::runtime_error("Audio playback requires a running script callback");
			const auto& root = ScriptRuntime::Get().Root();
			const auto relative = std::filesystem::u8path(name);
			if (name.empty() || relative.is_absolute() || relative.has_root_name() || relative.has_root_directory()) throw std::runtime_error("Audio expects an asset-relative path");
			for (const auto& part : relative) if (part == "..") throw std::runtime_error("Audio path escapes asset root");
			const auto full = std::filesystem::weakly_canonical(root / relative);
			const auto inside = full.lexically_relative(root);
			if (inside.empty() || inside.is_absolute() || *inside.begin() == ".." || !std::filesystem::is_regular_file(full)) throw std::runtime_error("Audio path is missing or escapes asset root");
			return full.generic_string();
		}
		static void AudioParameters(float volume, float pitch)
		{
			if (!std::isfinite(volume) || volume < 0.f || volume > 100.f || !std::isfinite(pitch) || pitch <= 0.f || pitch > 16.f)
				throw std::runtime_error("Audio volume must be 0..100 and pitch must be greater than 0 and at most 16");
		}
		void BindAudio(sol::state& lua)
		{
			sol::table audio = lua.create_table();
			audio["play"] = [this](const std::string& path, sol::optional<float> volume, sol::optional<float> pitch)
			{
				const auto v = volume.value_or(100.f), p = pitch.value_or(1.f);
				AudioParameters(v, p);
				const auto full = AudioPath(path);
				auto& manager = ly::AudioManager::GetAudioManager();
				sounds.erase(std::remove_if(sounds.begin(), sounds.end(), [&](auto id) { return !manager.IsSoundPlaying(id); }), sounds.end());
				if (sounds.size() >= 128) throw std::runtime_error("Too many simultaneous script sounds");
				const auto id = manager.PlaySoundTracked(full, AudioType::SFX_World, v, p);
				if (!id) throw std::runtime_error("Cannot load audio asset: " + path);
				sounds.push_back(id);
				return SoundHandle{ id };
			};
			environment["audio"] = audio;
			sol::table music = lua.create_table();
			music["play"] = [this](const std::string& path, sol::optional<float> volume, sol::optional<float> pitch)
			{
				const auto v = volume.value_or(100.f), p = pitch.value_or(1.f);
				AudioParameters(v, p);
				const auto full = AudioPath(path);
				auto& manager = ly::AudioManager::GetAudioManager();
				manager.PlayMusic(full, AudioType::Music, v, p);
				musicRevision = manager.MusicRevision();
			};
			music["fade"] = [this](const std::string& path, double seconds, sol::optional<float> volume, sol::optional<float> pitch)
			{
				const auto v = volume.value_or(100.f), p = pitch.value_or(1.f);
				AudioParameters(v, p);
				if (!std::isfinite(seconds) || seconds < 0 || seconds > 86400) throw std::runtime_error("Music fade duration must be 0..86400 seconds");
				const auto full = AudioPath(path);
				auto& manager = ly::AudioManager::GetAudioManager();
				manager.FadeToMusic(full, AudioType::Music, v, p, true, static_cast<float>(seconds));
				musicRevision = manager.MusicRevision();
			};
			music["stop"] = [this]()
			{
				if (ly::AudioManager::IsInitialized() && musicRevision && ly::AudioManager::GetAudioManager().MusicRevision() == musicRevision)
					ly::AudioManager::GetAudioManager().StopMusic();
				musicRevision = 0;
			};
			environment["music"] = music;
		}
		template<typename... Args>
		void Invoke(const sol::protected_function& function, Args&&... args)
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
		void Call(const char* name, Args&&... args)
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
		void Tick(float dt)
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
		void Contact(const char* name, ly::Actor* other) noexcept
		{
			if (!other || ended || !running) return;
			try { Call(name, ActorHandle{ other->GetWeakActor() }); }
			catch (const std::exception& exception) { calling = false; Fail(exception.what()); }
			catch (...) { calling = false; Fail("Unknown collision callback error"); }
		}
		TimerHandle Schedule(double seconds, sol::protected_function function, bool repeat)
		{
			if (!std::isfinite(seconds) || seconds < 0. || (repeat && seconds == 0.)) throw std::runtime_error("Invalid timer interval");
			if (ended || timers.size() >= 1024) throw std::runtime_error("Script timer limit reached or instance destroyed");
			auto active = std::make_shared<bool>(true);
			timers.push_back({ seconds, repeat ? seconds : 0., std::move(function), active });
			return TimerHandle{ active };
		}
		void BindWorld(sol::state& lua)
		{
			const auto owner = handle;
			sol::table world = lua.create_table();
			world["find"] = [owner](const std::string& name, sol::this_state state)
			{
				const auto actor = owner.Lock();
				if (actor && actor->GetWorld())
					for (const auto& weak : actor->GetWorld()->GetActorsByTypeIncludingPending<ly::Actor>())
						if (auto item = weak.lock(); item && !item->GetIsPendingDestroy())
							if (const auto* identity = item->FindComponent<ly::SceneEntityComponent>(); identity && identity->name == name)
								return sol::make_object(state, ActorHandle{ item });
				return sol::make_object(state, sol::nil);
			};
			world["player"] = [owner](sol::this_state state)
			{
				const auto actor = owner.Lock();
				const auto player = actor && actor->GetWorld() ? ScriptRuntime::Get().Player(*actor->GetWorld()) : nullptr;
				return player ? sol::make_object(state, ActorHandle{ player }) : sol::make_object(state, sol::nil);
			};
			world["find_all_by_type"] = [owner](const std::string& type, sol::this_state state)
			{
				auto result = sol::state_view{ state }.create_table();
				const auto actor = owner.Lock();
				int index = 1;
				if (actor && actor->GetWorld())
					for (const auto& weak : actor->GetWorld()->GetActorsByTypeIncludingPending<ly::Actor>())
						if (auto item = weak.lock(); item && !item->GetIsPendingDestroy() && Matches(*item, type)) result[index++] = ActorHandle{ item };
				return result;
			};
			world["find_in_radius"] = [owner](float x, float y, float radius, sol::optional<std::string> type, sol::this_state state)
			{
				auto result = sol::state_view{ state }.create_table();
				if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(radius) || radius < 0.f || radius > 1000000.f)
					throw std::runtime_error("Expected finite radius in [0, 1000000]");
				const auto actor = owner.Lock();
				int index = 1;
				const auto filter = type.value_or("");
				if (actor && actor->GetWorld()) actor->GetWorld()->ForEachActorInBounds({ { x - radius, y - radius }, { radius * 2.f, radius * 2.f } }, [&](ly::Actor& item)
				{
					const auto delta = item.GetActorLocation() - sf::Vector2f{ x, y };
					if (!item.GetIsPendingDestroy() && Matches(item, filter) && std::hypot(delta.x, delta.y) <= radius)
						result[index++] = ActorHandle{ item.GetWeakActor() };
					return true;
				});
				return result;
			};
			world["spawn"] = [owner](const std::string& source, float x, float y, sol::optional<sol::table> properties, sol::this_state state)
			{
				const auto actor = owner.Lock();
				if (!actor || !actor->GetWorld()) return sol::make_object(state, sol::nil);
				if (!std::isfinite(x) || !std::isfinite(y)) throw std::runtime_error("Expected finite spawn position");
				const bool prefab = source.size() >= 12 && source.substr(source.size() - 12) == ".prefab.json";
				if (prefab)
				{
					// Runtime failures must not add unresolved authored instances to the scene.
					const auto resolved = ly::PrefabLibrary::Get().Resolve(source);
					if (!resolved || !resolved.warnings.empty()) throw std::runtime_error(resolved.warnings.empty() ? "Cannot resolve prefab" : resolved.warnings.front());
				}
				const auto guid = ly::SceneEntityComponent::GenerateGuid();
				nlohmann::json values = nlohmann::json::object();
				if (properties) for (const auto& entry : *properties)
				{
					if (entry.first.get_type() != sol::type::string) throw std::runtime_error("Spawn properties require string keys");
					const auto value = FromLua(entry.second);
					if (!value) throw std::runtime_error("Unsupported spawn property value");
					values[entry.first.as<std::string>()] = std::visit([](const auto& v) -> nlohmann::json
					{
						using T = std::decay_t<decltype(v)>;
						if constexpr (std::is_same_v<T, sf::Vector2f>) return nlohmann::json::array({ v.x, v.y });
						else if constexpr (std::is_same_v<T, sf::Color>) return nullptr;
						else if constexpr (std::is_same_v<T, ly::ActorRef> || std::is_same_v<T, ly::reflect::PropertyList>) return nullptr;
						else if constexpr (std::is_same_v<T, float>)
						{
							// Scene enum/flag/int readers distinguish integral JSON values.
							if (std::trunc(v) == v && static_cast<double>(v) >= std::numeric_limits<int>::min() &&
								static_cast<double>(v) <= std::numeric_limits<int>::max()) return static_cast<int>(v);
							return v;
						}
						else return v;
					}, *value);
				}
				values["Location"] = { x, y };
				nlohmann::json entity{ { "guid", guid }, { "name", source }, { "properties", values } };
				entity[prefab ? "prefab" : "type"] = source;
				const auto loaded = ly::SceneSerializer::Load(*actor->GetWorld(), { { "format", "vega.scene" }, { "version", 1 },
					{ "settings", nlohmann::json::object() }, { "entities", nlohmann::json::array({ entity }) } });
				std::shared_ptr<ly::Actor> spawned;
				for (const auto& weak : actor->GetWorld()->GetActorsByTypeIncludingPending<ly::Actor>())
					if (auto item = weak.lock())
						if (const auto* identity = item->FindComponent<ly::SceneEntityComponent>(); identity && identity->guid == guid) { spawned = item; break; }
				if (!loaded.warnings.empty())
				{
					if (spawned) spawned->Destroy();
					throw std::runtime_error(loaded.warnings.front());
				}
				return spawned ? sol::make_object(state, ActorHandle{ spawned }) : sol::make_object(state, sol::nil);
			};
			environment["world"] = world;
		}
		void MakeEnvironment(sol::state& lua)
		{
			// No fallback to interpreter globals: even _G and mutable library tables are instance-local.
			environment = sol::environment{ lua, sol::create };
			lua_pushcfunction(lua.lua_state(), LuaPrint);
			environment["print"] = sol::stack::get<sol::function>(lua.lua_state(), -1);
			lua_pop(lua.lua_state(), 1);
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
			environment["log"] = environment["print"];
			const auto start = created;
			environment["time"] = [start]() { return std::chrono::duration<double>(Clock::now() - start).count(); };
			environment["vec"] = [](float x, float y) { return Vector{ x, y }; };
			handle = ActorHandle{ component.GetOwner().GetWeakActor() };
			BindWorld(lua);
			BindAudio(lua);
			const ActorHandle inputHandle = handle;
			sol::table input = lua.create_table();
			input["key_down"] = [inputHandle](const std::string& name) { return KeyDown(inputHandle, name); };
			keys.clear();
			const auto edge = [this](const std::string& name, bool pressed)
			{
				if (keys.size() >= 64 && keys.count(name) == 0) throw std::runtime_error("Too many watched input keys");
				auto entry = keys.find(name);
				if (entry == keys.end()) entry = keys.emplace(name, KeyState{ false, KeyDown(handle, name) }).first;
				const auto& value = entry->second;
				return component.GetWorld() && component.GetWorld()->HasInputFocus() &&
					(pressed ? value.current && !value.previous : !value.current && value.previous);
			};
			input["pressed"] = [edge](const std::string& name) { return edge(name, true); };
			input["released"] = [edge](const std::string& name) { return edge(name, false); };
			input["mouse_world"] = [inputHandle]()
			{
				const auto actor = inputHandle.Lock();
				const auto v = actor && actor->GetWorld() ? actor->GetWorld()->GetMouseWorldPosition() : sf::Vector2f{};
				return Vector{ v.x, v.y };
			};
			environment["input"] = input;
			sol::table timer = lua.create_table();
			timer["after"] = [this](double seconds, sol::protected_function fn) { return Schedule(seconds, std::move(fn), false); };
			timer["every"] = [this](double seconds, sol::protected_function fn) { return Schedule(seconds, std::move(fn), true); };
			environment["timer"] = timer;
			props = lua.create_table();
			self = lua.create_table_with("actor", handle, "props", props);
		}
		bool Load(sol::state& lua, bool start = true)
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
	bool ScriptComponent::Prepare() noexcept
	{
		if (HasBegunPlay()) return false;
		try { return mImpl->Load(ScriptRuntime::Get().mImpl->lua, false); }
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
					mImpl->props[entry.first] = ToLua(ScriptRuntime::Get().mImpl->lua, entry.second);
				}
			}
			return true;
		}
		catch (...) { return false; }
	}
	bool ScriptComponent::Reload() noexcept
	{
		try { return mImpl->Load(ScriptRuntime::Get().mImpl->lua); }
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
			if (ScriptRuntime::Get().HotReload() && Clock::now() >= mImpl->nextPoll)
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
	VEGA_REFLECT(ScriptComponent, ly::ActorComponent)
	{
		VEGA_REMOVABLE();
		VEGA_READONLY("Script", ScriptPath);
		VEGA_READONLY("Last Error", LastError);
	}

	void RegisterScriptTypes()
	{
		ly::reflect::RegisterEngineTypes();
		VEGA_REGISTER(ScriptComponent);
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
		});
	}
}
