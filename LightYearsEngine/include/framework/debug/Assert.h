#pragma once

#include "framework/debug/Log.h"

#include <string>

#ifndef LY_ENABLE_ASSERTS
	#if defined(NDEBUG)
		#define LY_ENABLE_ASSERTS 0
	#else
		#define LY_ENABLE_ASSERTS 1
	#endif
#endif

namespace ly::debug
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

#if LY_ENABLE_ASSERTS
	#define LY_ASSERT(condition, ...) \
		do \
		{ \
			if (!(condition)) \
			{ \
				::ly::debug::HandleAssertionFailure( \
					#condition, \
					LY_SOURCE_LOCATION, \
					::ly::debug::Log::Format(__VA_ARGS__) \
				); \
			} \
		} while (false)
#else
	#define LY_ASSERT(condition, ...) \
		do { (void)sizeof(condition); } while (false)
#endif

#define LY_CORE_ASSERT(condition, ...) LY_ASSERT(condition, __VA_ARGS__)

#if LY_ENABLE_ASSERTS
	#define LY_VERIFY(condition, ...) LY_ASSERT(condition, __VA_ARGS__)
#else
	#define LY_VERIFY(condition, ...) \
		do \
		{ \
			if (!(condition)) \
			{ \
				::ly::debug::HandleVerificationFailure( \
					#condition, \
					LY_SOURCE_LOCATION, \
					::ly::debug::Log::Format(__VA_ARGS__) \
				); \
			} \
		} while (false)
#endif

#define LY_DEBUG_BREAK() ::ly::debug::BreakOrAbort()
