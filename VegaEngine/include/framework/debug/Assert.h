#pragma once

#include "framework/debug/Log.h"

#include <string>

#ifndef VEGA_ENABLE_ASSERTS
	#if defined(NDEBUG)
		#define VEGA_ENABLE_ASSERTS 0
	#else
		#define VEGA_ENABLE_ASSERTS 1
	#endif
#endif

namespace vega::debug
{
	bool IsDebuggerAttached();
	[[noreturn]] void BreakOrAbort();
	[[noreturn]] void HandleAssertionFailure(
		const char* expression,
		SourceLocation location,
		std::string message
	);
	void HandleVerificationFailure(
		const char* expression,
		SourceLocation location,
		std::string message
	);
}

#if VEGA_ENABLE_ASSERTS
	#define VEGA_ASSERT(condition, ...) \
		do \
		{ \
			if (!(condition)) \
			{ \
				::vega::debug::HandleAssertionFailure( \
					#condition, \
					VEGA_SOURCE_LOCATION, \
					::vega::debug::Log::Format(__VA_ARGS__) \
				); \
			} \
		} while (false)
#else
	#define VEGA_ASSERT(condition, ...) \
		do { (void)sizeof(condition); } while (false)
#endif

#define VEGA_CORE_ASSERT(condition, ...) VEGA_ASSERT(condition, __VA_ARGS__)

#if VEGA_ENABLE_ASSERTS
	#define VEGA_VERIFY(condition, ...) VEGA_ASSERT(condition, __VA_ARGS__)
#else
	#define VEGA_VERIFY(condition, ...) \
		do \
		{ \
			if (!(condition)) \
			{ \
				::vega::debug::HandleVerificationFailure( \
					#condition, \
					VEGA_SOURCE_LOCATION, \
					::vega::debug::Log::Format(__VA_ARGS__) \
				); \
			} \
		} while (false)
#endif

#define VEGA_DEBUG_BREAK() ::vega::debug::BreakOrAbort()
