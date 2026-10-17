#pragma once

#include <cstdint>
#include <cstdio>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#ifndef VEGA_ENABLE_LOGGING
	#define VEGA_ENABLE_LOGGING 1
#endif

namespace vega::debug
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
		Game,
		Lua,
		Scene,
		Editor
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
		// Subscribers are independent of the legacy callback and file/stdout sinks.
		static std::uint64_t AddSink(Sink sink);
		static void RemoveSink(std::uint64_t id);
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

#define VEGA_SOURCE_LOCATION \
	::vega::debug::SourceLocation{ __FILE__, __func__, __LINE__ }

#if VEGA_ENABLE_LOGGING
	#define VEGA_CORE_TRACE(...) \
		::vega::debug::Log::WriteFormat(::vega::debug::LogLevel::Trace, ::vega::debug::LogChannel::Core, VEGA_SOURCE_LOCATION, __VA_ARGS__)
	#define VEGA_CORE_DEBUG(...) \
		::vega::debug::Log::WriteFormat(::vega::debug::LogLevel::Debug, ::vega::debug::LogChannel::Core, VEGA_SOURCE_LOCATION, __VA_ARGS__)
	#define VEGA_CORE_INFO(...) \
		::vega::debug::Log::WriteFormat(::vega::debug::LogLevel::Info, ::vega::debug::LogChannel::Core, VEGA_SOURCE_LOCATION, __VA_ARGS__)
	#define VEGA_CORE_WARN(...) \
		::vega::debug::Log::WriteFormat(::vega::debug::LogLevel::Warning, ::vega::debug::LogChannel::Core, VEGA_SOURCE_LOCATION, __VA_ARGS__)
	#define VEGA_CORE_ERROR(...) \
		::vega::debug::Log::WriteFormat(::vega::debug::LogLevel::Error, ::vega::debug::LogChannel::Core, VEGA_SOURCE_LOCATION, __VA_ARGS__)
	#define VEGA_CORE_FATAL(...) \
		::vega::debug::Log::WriteFormat(::vega::debug::LogLevel::Fatal, ::vega::debug::LogChannel::Core, VEGA_SOURCE_LOCATION, __VA_ARGS__)

	#define VEGA_GAME_TRACE(...) \
		::vega::debug::Log::WriteFormat(::vega::debug::LogLevel::Trace, ::vega::debug::LogChannel::Game, VEGA_SOURCE_LOCATION, __VA_ARGS__)
	#define VEGA_GAME_DEBUG(...) \
		::vega::debug::Log::WriteFormat(::vega::debug::LogLevel::Debug, ::vega::debug::LogChannel::Game, VEGA_SOURCE_LOCATION, __VA_ARGS__)
	#define VEGA_GAME_INFO(...) \
		::vega::debug::Log::WriteFormat(::vega::debug::LogLevel::Info, ::vega::debug::LogChannel::Game, VEGA_SOURCE_LOCATION, __VA_ARGS__)
	#define VEGA_GAME_WARN(...) \
		::vega::debug::Log::WriteFormat(::vega::debug::LogLevel::Warning, ::vega::debug::LogChannel::Game, VEGA_SOURCE_LOCATION, __VA_ARGS__)
	#define VEGA_GAME_ERROR(...) \
		::vega::debug::Log::WriteFormat(::vega::debug::LogLevel::Error, ::vega::debug::LogChannel::Game, VEGA_SOURCE_LOCATION, __VA_ARGS__)
	#define VEGA_GAME_FATAL(...) \
		::vega::debug::Log::WriteFormat(::vega::debug::LogLevel::Fatal, ::vega::debug::LogChannel::Game, VEGA_SOURCE_LOCATION, __VA_ARGS__)
#else
	#define VEGA_CORE_TRACE(...) ((void)0)
	#define VEGA_CORE_DEBUG(...) ((void)0)
	#define VEGA_CORE_INFO(...) ((void)0)
	#define VEGA_CORE_WARN(...) ((void)0)
	#define VEGA_CORE_ERROR(...) ((void)0)
	#define VEGA_CORE_FATAL(...) ((void)0)
	#define VEGA_GAME_TRACE(...) ((void)0)
	#define VEGA_GAME_DEBUG(...) ((void)0)
	#define VEGA_GAME_INFO(...) ((void)0)
	#define VEGA_GAME_WARN(...) ((void)0)
	#define VEGA_GAME_ERROR(...) ((void)0)
	#define VEGA_GAME_FATAL(...) ((void)0)
#endif
