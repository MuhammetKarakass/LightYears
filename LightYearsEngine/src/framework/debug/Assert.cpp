#include "framework/debug/Assert.h"

#include <cstdlib>

#if defined(_WIN32)
	#include <Windows.h>
	#include <intrin.h>
#endif

namespace ly::debug
{
	bool IsDebuggerAttached()
	{
#if defined(_WIN32)
		return IsDebuggerPresent() != FALSE;
#else
		return false;
#endif
	}

	[[noreturn]] void BreakOrAbort()
	{
#if defined(_WIN32)
		if (IsDebuggerAttached())
		{
			__debugbreak();
		}
#endif
		std::abort();
	}

	[[noreturn]] void HandleAssertionFailure(
		const char* expression,
		SourceLocation location,
		std::string message
	)
	{
		Log::Write(
			LogLevel::Fatal,
			LogChannel::Core,
			location,
			"Assertion failed: " + std::string{ expression ? expression : "" } +
				(message.empty() ? std::string{} : " | " + message)
		);
		Log::Flush();
		BreakOrAbort();
	}

	void HandleVerificationFailure(
		const char* expression,
		SourceLocation location,
		std::string message
	)
	{
		Log::Write(
			LogLevel::Error,
			LogChannel::Core,
			location,
			"Verification failed: " +
				std::string{ expression ? expression : "" } +
				(message.empty() ? std::string{} : " | " + message)
		);
	}
}
