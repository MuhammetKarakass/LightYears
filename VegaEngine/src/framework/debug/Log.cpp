#include "framework/debug/Log.h"

#include <chrono>
#include <algorithm>
#include <condition_variable>
#include <ctime>
#include <deque>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>

namespace vega::debug
{
	namespace
	{
		std::mutex gLogMutex;
		std::ofstream gLogFile;
		Log::Sink gSink;
		std::vector<std::pair<std::uint64_t, Log::Sink>> gSubscribers;
		std::uint64_t gNextSink = 0;
		bool gInitialized = false;
#if defined(NDEBUG)
		LogLevel gMinimumLevel = LogLevel::Warning;
#else
		LogLevel gMinimumLevel = LogLevel::Trace;
#endif

		// Console output goes through a background thread. Writing to a Windows console is a round trip to the
		// console host for every write; when that process is short of CPU, each log line held the game thread for
		// tens of milliseconds (60-150 ms frames while other programs were busy). The file sink stays synchronous.
		// Lines keep their order; an Error line waits until everything before it has reached the console.
		class ConsoleWriter
		{
		public:
			~ConsoleWriter()
			{
				Stop();
			}

			void Push(std::string line, bool error)
			{
				{
					std::lock_guard<std::mutex> lock{ mMutex };
					if (mStopped)
					{
						(error ? std::cerr : std::cout) << line << '\n';
						return;
					}
					if (!mThread.joinable())
					{
						mThread = std::thread([this]() { Run(); });
					}
					if (mQueue.size() >= MaxQueuedLines)
					{
						++mDropped;
						return;
					}
					mQueue.push_back({ std::move(line), error });
				}
				mWake.notify_one();
			}

			// After Log::Shutdown a new Log::Initialize may start writing again.
			void Restart()
			{
				std::lock_guard<std::mutex> lock{ mMutex };
				if (!mThread.joinable())
				{
					mStopped = false;
					mStopRequested = false;
				}
			}

			void Drain()
			{
				std::unique_lock<std::mutex> lock{ mMutex };
				mDrained.wait(lock, [this]() { return (mQueue.empty() && !mWriting) || !mThread.joinable(); });
			}

			void Stop()
			{
				{
					std::lock_guard<std::mutex> lock{ mMutex };
					mStopRequested = true;
				}
				mWake.notify_one();
				if (mThread.joinable())
				{
					mThread.join();
				}
				std::lock_guard<std::mutex> lock{ mMutex };
				mStopped = true;
			}

		private:
			struct Line
			{
				std::string text;
				bool error = false;
			};

			static constexpr std::size_t MaxQueuedLines = 20000;

			void Run()
			{
				std::deque<Line> batch;
				for (;;)
				{
					std::size_t dropped = 0;
					{
						std::unique_lock<std::mutex> lock{ mMutex };
						mWake.wait(lock, [this]() { return !mQueue.empty() || mStopRequested; });
						if (mQueue.empty())
						{
							mDrained.notify_all();
							return;
						}
						batch.swap(mQueue);
						dropped = mDropped;
						mDropped = 0;
						mWriting = true;
					}
					// One write per stream and batch instead of two per line.
					std::string out;
					std::string err;
					for (const Line& line : batch)
					{
						std::string& target = line.error ? err : out;
						target += line.text;
						target += '\n';
					}
					if (dropped > 0)
					{
						out += "[log] " + std::to_string(dropped) + " console lines dropped; the log file has them all\n";
					}
					if (!out.empty())
					{
						std::cout.write(out.data(), static_cast<std::streamsize>(out.size()));
						std::cout.flush();
					}
					if (!err.empty())
					{
						std::cerr.write(err.data(), static_cast<std::streamsize>(err.size()));
						std::cerr.flush();
					}
					batch.clear();
					{
						std::lock_guard<std::mutex> lock{ mMutex };
						mWriting = false;
					}
					mDrained.notify_all();
				}
			}

			std::mutex mMutex;
			std::condition_variable mWake;
			std::condition_variable mDrained;
			std::deque<Line> mQueue;
			std::size_t mDropped = 0;
			bool mWriting = false;
			bool mStopRequested = false;
			bool mStopped = false;
			std::thread mThread;
		};

		ConsoleWriter& Console()
		{
			static ConsoleWriter writer;
			return writer;
		}

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
		Console().Restart();
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
		Console().Stop();
		std::lock_guard<std::mutex> lock{ gLogMutex };
		if (gLogFile.is_open())
		{
			gLogFile.flush();
			gLogFile.close();
		}
		gSink = {};
		gSubscribers.clear();
		gInitialized = false;
	}

	void Log::Flush()
	{
		Console().Drain();
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

	std::uint64_t Log::AddSink(Sink sink)
	{
		std::lock_guard<std::mutex> lock{ gLogMutex };
		const auto id = ++gNextSink;
		gSubscribers.emplace_back(id, std::move(sink));
		return id;
	}

	void Log::RemoveSink(std::uint64_t id)
	{
		std::lock_guard<std::mutex> lock{ gLogMutex };
		gSubscribers.erase(std::remove_if(gSubscribers.begin(), gSubscribers.end(),
			[id](const auto& entry) { return entry.first == id; }), gSubscribers.end());
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
		case LogChannel::Lua: return "Lua";
		case LogChannel::Scene: return "Scene";
		case LogChannel::Editor: return "Editor";
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
		std::vector<Sink> subscribers;
		const bool error = level >= LogLevel::Error;
		{
			std::lock_guard<std::mutex> lock{ gLogMutex };
			std::string line = BuildLine(record);
			if (gLogFile.is_open())
			{
				gLogFile << line << '\n';
				if (error)
				{
					gLogFile.flush();
				}
			}
			Console().Push(std::move(line), error);
			sink = gSink;
			for (const auto& entry : gSubscribers) subscribers.push_back(entry.second);
		}
		if (error)
		{
			// Errors reach the console before the caller continues, as before, so a crash right after keeps them.
			Console().Drain();
		}
		// Callbacks run outside the logger lock and may log or unsubscribe.
		for (const auto& subscriber : subscribers)
		{
			if (subscriber) subscriber(record);
		}
		if (sink)
		{
			sink(record);
		}
	}
}
