#pragma once

#include <cstdint>
#include <cstdio>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#ifndef LY_ENABLE_LOGGING
	#define LY_ENABLE_LOGGING 1
#endif

namespace ly::debug
{
	enum class LogLevel : std::uint8_t
	{
		Trace,
		Debug,
		Info,
		Warning,
		Error,
		Fatal,
		Off
	};

	enum class LogChannel : std::uint8_t
	{
		Core,
		Game
	};

	struct SourceLocation
	{
		const char* file = "";
		const char* function = "";
		int line = 0;
	};

	struct LogRecord
	{
		LogLevel level = LogLevel::Info;
		LogChannel channel = LogChannel::Core;
		std::string message;
		SourceLocation location;
		std::string timestamp;
	};

	class Log
	{
	public:
		using Sink = std::function<void(const LogRecord&)>;

		static void Initialize(const std::string& filePath = {});
		static void Shutdown();
		static void Flush();

		static void SetMinimumLevel(LogLevel level);
		static LogLevel GetMinimumLevel();
		static bool ShouldLog(LogLevel level);

		static void SetSink(Sink sink);
		static const char* GetLevelName(LogLevel level);
		static const char* GetChannelName(LogChannel channel);

		static void Write(
			LogLevel level,
			LogChannel channel,
			SourceLocation location,
			std::string message
		);

		template<typename... Args>
		static std::string Format(const char* format, Args&&... args)
		{
			if (!format)
			{
				return {};
			}
			const int required = std::snprintf(
				nullptr,
				0,
				format,
				std::forward<Args>(args)...
			);
			if (required <= 0)
			{
				return std::string{ format };
			}
			std::vector<char> buffer(static_cast<std::size_t>(required) + 1u);
			std::snprintf(
				buffer.data(),
				buffer.size(),
				format,
				std::forward<Args>(args)...
			);
			return std::string{ buffer.data(), static_cast<std::size_t>(required) };
		}

		template<typename... Args>
		static void WriteFormat(
			LogLevel level,
			LogChannel channel,
			SourceLocation location,
			const char* format,
			Args&&... args
		)
		{
			if (!ShouldLog(level))
			{
				return;
			}
			Write(
				level,
				channel,
				location,
				Format(format, std::forward<Args>(args)...)
			);
		}
	};
}

#define LY_SOURCE_LOCATION \
	::ly::debug::SourceLocation{ __FILE__, __func__, __LINE__ }

#if LY_ENABLE_LOGGING
	#define LY_CORE_TRACE(...) \
		::ly::debug::Log::WriteFormat(::ly::debug::LogLevel::Trace, ::ly::debug::LogChannel::Core, LY_SOURCE_LOCATION, __VA_ARGS__)
	#define LY_CORE_DEBUG(...) \
		::ly::debug::Log::WriteFormat(::ly::debug::LogLevel::Debug, ::ly::debug::LogChannel::Core, LY_SOURCE_LOCATION, __VA_ARGS__)
	#define LY_CORE_INFO(...) \
		::ly::debug::Log::WriteFormat(::ly::debug::LogLevel::Info, ::ly::debug::LogChannel::Core, LY_SOURCE_LOCATION, __VA_ARGS__)
	#define LY_CORE_WARN(...) \
		::ly::debug::Log::WriteFormat(::ly::debug::LogLevel::Warning, ::ly::debug::LogChannel::Core, LY_SOURCE_LOCATION, __VA_ARGS__)
	#define LY_CORE_ERROR(...) \
		::ly::debug::Log::WriteFormat(::ly::debug::LogLevel::Error, ::ly::debug::LogChannel::Core, LY_SOURCE_LOCATION, __VA_ARGS__)
	#define LY_CORE_FATAL(...) \
		::ly::debug::Log::WriteFormat(::ly::debug::LogLevel::Fatal, ::ly::debug::LogChannel::Core, LY_SOURCE_LOCATION, __VA_ARGS__)

	#define LY_GAME_TRACE(...) \
		::ly::debug::Log::WriteFormat(::ly::debug::LogLevel::Trace, ::ly::debug::LogChannel::Game, LY_SOURCE_LOCATION, __VA_ARGS__)
	#define LY_GAME_DEBUG(...) \
		::ly::debug::Log::WriteFormat(::ly::debug::LogLevel::Debug, ::ly::debug::LogChannel::Game, LY_SOURCE_LOCATION, __VA_ARGS__)
	#define LY_GAME_INFO(...) \
		::ly::debug::Log::WriteFormat(::ly::debug::LogLevel::Info, ::ly::debug::LogChannel::Game, LY_SOURCE_LOCATION, __VA_ARGS__)
	#define LY_GAME_WARN(...) \
		::ly::debug::Log::WriteFormat(::ly::debug::LogLevel::Warning, ::ly::debug::LogChannel::Game, LY_SOURCE_LOCATION, __VA_ARGS__)
	#define LY_GAME_ERROR(...) \
		::ly::debug::Log::WriteFormat(::ly::debug::LogLevel::Error, ::ly::debug::LogChannel::Game, LY_SOURCE_LOCATION, __VA_ARGS__)
	#define LY_GAME_FATAL(...) \
		::ly::debug::Log::WriteFormat(::ly::debug::LogLevel::Fatal, ::ly::debug::LogChannel::Game, LY_SOURCE_LOCATION, __VA_ARGS__)
#else
	#define LY_CORE_TRACE(...) ((void)0)
	#define LY_CORE_DEBUG(...) ((void)0)
	#define LY_CORE_INFO(...) ((void)0)
	#define LY_CORE_WARN(...) ((void)0)
	#define LY_CORE_ERROR(...) ((void)0)
	#define LY_CORE_FATAL(...) ((void)0)
	#define LY_GAME_TRACE(...) ((void)0)
	#define LY_GAME_DEBUG(...) ((void)0)
	#define LY_GAME_INFO(...) ((void)0)
	#define LY_GAME_WARN(...) ((void)0)
	#define LY_GAME_ERROR(...) ((void)0)
	#define LY_GAME_FATAL(...) ((void)0)
#endif
