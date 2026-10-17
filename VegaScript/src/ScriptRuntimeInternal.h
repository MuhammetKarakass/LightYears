#pragma once

#include "script/ScriptRuntime.h"
#include <sol/sol.hpp>
#include <cstddef>
#include <cstdint>

namespace vega::script
{
	namespace detail
	{
		struct RuntimeLimits
		{
			std::size_t memoryLimit = 256u * 1024u * 1024u;
			std::size_t memoryUsed = 0;
			std::uint64_t instructionBudget = 10000000;
			std::uint64_t remaining = 0;
			int hookCount = 1000;
			unsigned int depth = 0;
		};
		void* Allocate(void* user, void* pointer, std::size_t oldSize, std::size_t newSize);
		void InstructionHook(lua_State* state, lua_Debug* debug);
		struct InstructionScope
		{
			lua_State* state;
			explicit InstructionScope(lua_State* lua);
			~InstructionScope();
		};
		bool DeviceKey(const std::string& name);
	}

	using detail::InstructionScope;

	struct ScriptRuntime::Impl
	{
		// The allocator's accounting must outlive the interpreter, including lua_close.
		detail::RuntimeLimits limits;
		sol::state lua{ sol::default_at_panic, detail::Allocate, &limits };
		std::filesystem::path root;
		PlayerResolver player;
		KeyReader keyReader;
#ifdef VEGA_SCRIPT_RELEASE
		bool hotReload = false;
#else
		bool hotReload = true;
#endif
		Impl();
	};
}
