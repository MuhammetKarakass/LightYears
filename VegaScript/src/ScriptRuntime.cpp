#include "framework/EngineContext.h"
#include "script/ScriptComponent.h"
#include "script/ScriptRuntime.h"
#include "ScriptBindingsInternal.h"
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

#include "ScriptBindingsInternal.h"
#include "ScriptRuntimeInternal.h"

namespace vega::script
{
	using namespace detail;
	namespace detail
	{
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
		InstructionScope::InstructionScope(lua_State* lua) : state{ lua }
		{
			void* user = nullptr;
			lua_getallocf(state, &user);
			auto& limits = *static_cast<RuntimeLimits*>(user);
			if (limits.depth++ != 0) return;
			limits.remaining = limits.instructionBudget;
			limits.hookCount = static_cast<int>(std::min<std::uint64_t>(1000, limits.instructionBudget));
			lua_sethook(state, InstructionHook, LUA_MASKCOUNT, limits.hookCount);
		}
		InstructionScope::~InstructionScope()
		{
			void* user = nullptr;
			lua_getallocf(state, &user);
			if (--static_cast<RuntimeLimits*>(user)->depth == 0) lua_sethook(state, nullptr, 0, 0);
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
	}

	ScriptRuntime::Impl::Impl()
	{
		lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);
		detail::Bind(lua);
	}
	ScriptRuntime::ScriptRuntime() : mImpl{ std::make_unique<Impl>() } {}
	ScriptRuntime::~ScriptRuntime() = default;
	ScriptRuntime& ScriptRuntime::Get() { return Get(vega::EngineContext::Current()); }
	ScriptRuntime& ScriptRuntime::Get(vega::EngineContext& context)
	{
		auto& slot = context.Scripts();
		if (!slot) slot.reset(new ScriptRuntime);
		return *slot;
	}
	void ScriptRuntime::SetRoot(const std::filesystem::path& root) { mImpl->root = std::filesystem::weakly_canonical(root); }
	void ScriptRuntime::SetHotReload(bool enabled) { mImpl->hotReload = enabled; }
	bool ScriptRuntime::HotReload() const { return mImpl->hotReload; }
	void ScriptRuntime::SetInstructionBudget(std::uint64_t instructions) { mImpl->limits.instructionBudget = std::max<std::uint64_t>(1, instructions); }
	void ScriptRuntime::SetMemoryLimit(std::size_t bytes) { mImpl->limits.memoryLimit = bytes; }
	const std::filesystem::path& ScriptRuntime::Root() const { return mImpl->root; }
	void ScriptRuntime::SetPlayerResolver(PlayerResolver resolver) { mImpl->player = std::move(resolver); }
	std::shared_ptr<vega::Actor> ScriptRuntime::Player(vega::World& world) const
	{
		const auto actor = mImpl->player ? mImpl->player(world).lock() : nullptr;
		return actor && actor->GetWorld() == &world && !actor->GetIsPendingDestroy() ? actor : nullptr;
	}
	void ScriptRuntime::SetKeyReader(KeyReader reader) { mImpl->keyReader = std::move(reader); }
	bool ScriptRuntime::ReadKey(const std::string& key) const { return mImpl->keyReader ? mImpl->keyReader(key) : DeviceKey(key); }
	// Ownerless game setup remains a compatibility entry boundary.
	void SetScriptRoot(const std::filesystem::path& root) { ScriptRuntime::Get(vega::EngineContext::Current()).SetRoot(root); }
}
