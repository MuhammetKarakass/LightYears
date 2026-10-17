#include "framework/EngineContext.h"
#include "script/ScriptComponent.h"
#include "script/ScriptRuntime.h"
#include "ScriptComponentInternal.h"
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

#include "ScriptBindingsInternal.h"
#include "ScriptComponentInternal.h"

namespace vega::script
{
	using namespace detail;
	namespace detail
	{
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
		bool Matches(const ly::Actor& actor, const std::string& name)
		{
			if (name.empty()) return true;
			for (auto* type = actor.GetEngineContext().Types().FindMostDerived(actor); type; type = type->base)
				if (type->name == name) return true;
			return false;
		}
		nlohmann::json MessageData(const sol::object& object, unsigned int& entries, unsigned int depth)
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
		sol::object ToLua(sol::state_view lua, const Value& value, ly::World* world)
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
			if (property.kind == PropertyKind::Flags && input.get_type() == sol::type::string)
			{
				int bits = 0;
				bool legacy = false;
				if (!property.ParseFlags(input.as<std::string>(), bits, &legacy)) return {};
				if (legacy) LY_CORE_WARN("Lua %s: legacy flags label accepted; use project labels", property.name.c_str());
				return bits;
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
			return ScriptRuntime::Get(actor->GetEngineContext()).ReadKey(name);
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
					return ReadProperty(state, component, component ? component->GetOwner().GetEngineContext().Types().FindMostDerived(*component) : nullptr, name);
				},
				"set", [](const ComponentHandle& handle, const std::string& name, sol::object value)
				{
					const auto actor = handle.owner.Lock();
					auto* component = handle.Resolve(actor);
					return WriteProperty(component, component ? component->GetOwner().GetEngineContext().Types().FindMostDerived(*component) : nullptr, name, value);
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
					const auto* type = a ? a->GetEngineContext().Types().FindMostDerived(*a) : nullptr;
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
					return ReadProperty(s, a.get(), a ? a->GetEngineContext().Types().FindMostDerived(*a) : nullptr, name);
				},
				"set", [](const ActorHandle& h, const std::string& name, sol::object value)
				{
					const auto a = h.Lock();
					return WriteProperty(a.get(), a ? a->GetEngineContext().Types().FindMostDerived(*a) : nullptr, name, value);
				},
				"component", [](const ActorHandle& h, const std::string& name, sol::this_state s)
				{
					const auto a = h.Lock();
					sol::object result = sol::make_object(s, sol::nil);

					if (a) a->ForEachComponent([&](ly::ActorComponent& component)
					{
						for (auto* type = component.GetOwner().GetEngineContext().Types().FindMostDerived(component); type; type = type->base)
							if (result == sol::nil && type->name == name) result = sol::make_object(s, ComponentHandle{ h, *ly::IdentifyComponent(*a, component) });

					});
					return result;
				});
		}
	}

	std::string ScriptComponent::Impl::AudioPath(const std::string& name) const
	{
		if (!running || ended || component.GetOwner().GetIsPendingDestroy()) throw std::runtime_error("Audio playback requires a running script callback");
		const auto& root = runtime.Root();
		const auto relative = std::filesystem::u8path(name);
		if (name.empty() || relative.is_absolute() || relative.has_root_name() || relative.has_root_directory()) throw std::runtime_error("Audio expects an asset-relative path");
		for (const auto& part : relative) if (part == "..") throw std::runtime_error("Audio path escapes asset root");
		const auto full = std::filesystem::weakly_canonical(root / relative);
		const auto inside = full.lexically_relative(root);
		if (inside.empty() || inside.is_absolute() || *inside.begin() == ".." || !std::filesystem::is_regular_file(full)) throw std::runtime_error("Audio path is missing or escapes asset root");
		return full.generic_string();
	}
	void ScriptComponent::Impl::AudioParameters(float volume, float pitch)
	{
		if (!std::isfinite(volume) || volume < 0.f || volume > 100.f || !std::isfinite(pitch) || pitch <= 0.f || pitch > 16.f)
			throw std::runtime_error("Audio volume must be 0..100 and pitch must be greater than 0 and at most 16");
	}
	void ScriptComponent::Impl::BindAudio(sol::state& lua)
	{
		sol::table audio = lua.create_table();
		audio["play"] = [this](const std::string& path, sol::optional<float> volume, sol::optional<float> pitch)
		{
			const auto v = volume.value_or(100.f), p = pitch.value_or(1.f);
			AudioParameters(v, p);
			const auto full = AudioPath(path);
			auto& manager = component.GetOwner().GetEngineContext().Audio();
			sounds.erase(std::remove_if(sounds.begin(), sounds.end(), [&](auto id) { return !manager.IsSoundPlaying(id); }), sounds.end());
			if (sounds.size() >= 128) throw std::runtime_error("Too many simultaneous script sounds");
			const auto id = manager.PlaySoundTracked(full, AudioType::SFX_World, v, p);
			if (!id) throw std::runtime_error("Cannot load audio asset: " + path);
			sounds.push_back(id);
			return SoundHandle{ id, &component.GetOwner().GetEngineContext() };
		};
		environment["audio"] = audio;
		sol::table music = lua.create_table();
		music["play"] = [this](const std::string& path, sol::optional<float> volume, sol::optional<float> pitch)
		{
			const auto v = volume.value_or(100.f), p = pitch.value_or(1.f);
			AudioParameters(v, p);
			const auto full = AudioPath(path);
			auto& manager = component.GetOwner().GetEngineContext().Audio();
			manager.PlayMusic(full, AudioType::Music, v, p);
			musicRevision = manager.MusicRevision();
		};
		music["fade"] = [this](const std::string& path, double seconds, sol::optional<float> volume, sol::optional<float> pitch)
		{
			const auto v = volume.value_or(100.f), p = pitch.value_or(1.f);
			AudioParameters(v, p);
			if (!std::isfinite(seconds) || seconds < 0 || seconds > 86400) throw std::runtime_error("Music fade duration must be 0..86400 seconds");
			const auto full = AudioPath(path);
			auto& manager = component.GetOwner().GetEngineContext().Audio();
			manager.FadeToMusic(full, AudioType::Music, v, p, true, static_cast<float>(seconds));
			musicRevision = manager.MusicRevision();
		};
		music["stop"] = [this]()
		{
			if (component.GetOwner().GetEngineContext().HasAudio() && musicRevision && component.GetOwner().GetEngineContext().Audio().MusicRevision() == musicRevision)
				component.GetOwner().GetEngineContext().Audio().StopMusic();
			musicRevision = 0;
		};
		environment["music"] = music;
	}
	TimerHandle ScriptComponent::Impl::Schedule(double seconds, sol::protected_function function, bool repeat)
	{
		if (!std::isfinite(seconds) || seconds < 0. || (repeat && seconds == 0.)) throw std::runtime_error("Invalid timer interval");
		if (ended || timers.size() >= 1024) throw std::runtime_error("Script timer limit reached or instance destroyed");
		auto active = std::make_shared<bool>(true);
		timers.push_back({ seconds, repeat ? seconds : 0., std::move(function), active });
		return TimerHandle{ active };
	}
	void ScriptComponent::Impl::BindWorld(sol::state& lua)
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
			const auto player = actor && actor->GetWorld() ? ScriptRuntime::Get(actor->GetEngineContext()).Player(*actor->GetWorld()) : nullptr;
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
				const auto resolved = actor->GetEngineContext().Prefabs().Resolve(source);
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
	void ScriptComponent::Impl::MakeEnvironment(sol::state& lua)
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
}
