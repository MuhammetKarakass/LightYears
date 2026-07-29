#include "framework/debug/Log.h"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>

namespace ly::debug
{
	namespace
	{
		std::mutex gLogMutex;
		std::ofstream gLogFile;
		Log::Sink gSink;
		bool gInitialized = false;
#if defined(NDEBUG)
		LogLevel gMinimumLevel = LogLevel::Warning;
#else
		LogLevel gMinimumLevel = LogLevel::Trace;
#endif

		std::string BuildTimestamp()
		{
			const auto now = std::chrono::system_clock::now();
			const auto milliseconds =
				std::chrono::duration_cast<std::chrono::milliseconds>(
					now.time_since_epoch()
				) % 1000;
			const std::time_t time = std::chrono::system_clock::to_time_t(now);
			std::tm localTime{};
#if defined(_WIN32)
			localtime_s(&localTime, &time);
#else
			localtime_r(&time, &localTime);
#endif
			std::ostringstream stream;
			stream << std::put_time(&localTime, "%H:%M:%S")
				<< '.' << std::setfill('0') << std::setw(3)
				<< milliseconds.count();
			return stream.str();
		}

		std::string BuildLine(const LogRecord& record)
		{
			std::ostringstream stream;
			stream << '[' << record.timestamp << "] ["
				<< Log::GetChannelName(record.channel) << "] ["
				<< Log::GetLevelName(record.level) << "] "
				<< record.message;
			if (record.location.file && record.location.file[0] != '\0')
			{
				stream << " (" << record.location.file << ':'
					<< record.location.line << ')';
			}
			return stream.str();
		}
	}

	void Log::Initialize(const std::string& filePath)
	{
		std::lock_guard<std::mutex> lock{ gLogMutex };
		if (gInitialized)
		{
			return;
		}
		if (!filePath.empty())
		{
			gLogFile.open(filePath, std::ios::out | std::ios::trunc);
		}
		gInitialized = true;
	}

	void Log::Shutdown()
	{
		std::lock_guard<std::mutex> lock{ gLogMutex };
		if (gLogFile.is_open())
		{
			gLogFile.flush();
			gLogFile.close();
		}
		gSink = {};
		gInitialized = false;
	}

	void Log::Flush()
	{
		std::lock_guard<std::mutex> lock{ gLogMutex };
		std::cout.flush();
		std::cerr.flush();
		if (gLogFile.is_open())
		{
			gLogFile.flush();
		}
	}

	void Log::SetMinimumLevel(LogLevel level)
	{
		std::lock_guard<std::mutex> lock{ gLogMutex };
		gMinimumLevel = level;
	}

	LogLevel Log::GetMinimumLevel()
	{
		std::lock_guard<std::mutex> lock{ gLogMutex };
		return gMinimumLevel;
	}

	bool Log::ShouldLog(LogLevel level)
	{
		std::lock_guard<std::mutex> lock{ gLogMutex };
		return level >= gMinimumLevel && gMinimumLevel != LogLevel::Off;
	}

	void Log::SetSink(Sink sink)
	{
		std::lock_guard<std::mutex> lock{ gLogMutex };
		gSink = std::move(sink);
	}

	const char* Log::GetLevelName(LogLevel level)
	{
		switch (level)
		{
		case LogLevel::Trace: return "TRACE";
		case LogLevel::Debug: return "DEBUG";
		case LogLevel::Info: return "INFO";
		case LogLevel::Warning: return "WARN";
		case LogLevel::Error: return "ERROR";
		case LogLevel::Fatal: return "FATAL";
		case LogLevel::Off: return "OFF";
		}
		return "UNKNOWN";
	}

	const char* Log::GetChannelName(LogChannel channel)
	{
		switch (channel)
		{
		case LogChannel::Core: return "CORE";
		case LogChannel::Game: return "GAME";
		}
		return "UNKNOWN";
	}

	void Log::Write(
		LogLevel level,
		LogChannel channel,
		SourceLocation location,
		std::string message
	)
	{
		LogRecord record{
			level,
			channel,
			std::move(message),
			location,
			BuildTimestamp()
		};
		Sink sink;
		{
			std::lock_guard<std::mutex> lock{ gLogMutex };
			const std::string line = BuildLine(record);
			std::ostream& console =
				level >= LogLevel::Error ? std::cerr : std::cout;
			console << line << '\n';
			if (gLogFile.is_open())
			{
				gLogFile << line << '\n';
			}
			if (level >= LogLevel::Error)
			{
				console.flush();
				if (gLogFile.is_open())
				{
					gLogFile.flush();
				}
			}
			sink = gSink;
		}
		if (sink)
		{
			sink(record);
		}
	}
}
