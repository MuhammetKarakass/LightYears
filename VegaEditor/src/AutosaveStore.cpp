#include "editor/AutosaveStore.h"
#include <chrono>
#include <fstream>
#include <algorithm>
#include <cctype>
#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#else
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace vega
{
	struct AutosaveStore::Lock
	{
#ifdef _WIN32
		HANDLE file = INVALID_HANDLE_VALUE;
		explicit Lock(const std::filesystem::path& path)
		{
			file = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		}
		bool Valid() const { return file != INVALID_HANDLE_VALUE; }
		~Lock() { if (Valid()) CloseHandle(file); }
#else
		int file = -1;
		explicit Lock(const std::filesystem::path& path)
		{
			file = open(path.c_str(), O_CREAT | O_RDWR, 0600);
			if (file >= 0 && flock(file, LOCK_EX | LOCK_NB) != 0) { close(file); file = -1; }
		}
		bool Valid() const { return file >= 0; }
		~Lock() { if (Valid()) close(file); }
#endif
	};

	namespace
	{
		std::string Read(const std::filesystem::path& path)
		{
			std::ifstream input{ path, std::ios::binary };
			return { std::istreambuf_iterator<char>{ input }, {} };
		}
		std::string Summary(const std::filesystem::path& source, const std::string& text)
		{
			if (!std::filesystem::exists(source)) return "Original file is missing; recover as an unsaved document.";
			try
			{
				const auto changes = Json::diff(Json::parse(Read(source)), Json::parse(text));
				std::string summary = std::to_string(changes.size()) + " JSON changes compared with disk";
				for (std::size_t index = 0; index < std::min<std::size_t>(changes.size(), 8); ++index)
					summary += "\n" + changes[index].at("op").get<std::string>() + " " + changes[index].at("path").get<std::string>();
				if (changes.size() > 8) summary += "\n...";
				return summary;
			}
			catch (const std::exception&) { return "Original JSON is unreadable; recovery opens the snapshot without replacing it."; }
		}
		bool SafeSource(const JsonDocument& resolver, const std::filesystem::path& source, std::filesystem::path& absolute)
		{
			const auto relative = source.is_absolute() ? source.lexically_relative(resolver.Root()) : source;
			if (relative.empty()) return false;
			for (const auto& part : relative)
			{
				auto name = part.u8string();
				std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
				if (name == ".." || name == ".vega") return false;
			}
			return resolver.Resolve(relative, absolute) && absolute != resolver.Root();
		}
	}

	AutosaveStore::AutosaveStore() = default;
	AutosaveStore::~AutosaveStore()
	{
		if (Enabled() && !mSimulateUnclean)
		{
			std::string ignored;
			JsonDocument resolver{ mRoot };
			std::filesystem::path safe;
			if (resolver.Root() == mRoot && resolver.Resolve((mSession / "clean.json").lexically_relative(mRoot), safe) && safe == mSession / "clean.json")
				AtomicWriteText(safe, "true\n", ignored);
		}
	}

	bool AutosaveStore::Begin(const std::filesystem::path& root)
	{
		if (Enabled()) { mError = "Autosave session already started"; return false; }
		try
		{
			JsonDocument resolver{ root };
			std::filesystem::path directory;
			if (resolver.Root().empty() || !resolver.Resolve(".vega/Autosave", directory)) throw std::runtime_error("Autosave directory escapes the asset root");
			std::filesystem::create_directories(directory);
			const auto stamp = std::chrono::system_clock::now().time_since_epoch().count();
			for (unsigned int attempt = 0; attempt < 100; ++attempt)
			{
				const auto session = directory / (std::to_string(stamp) + "-" + std::to_string(attempt));
				if (!std::filesystem::create_directory(session)) continue;
				auto lock = std::make_unique<Lock>(session / "session.lock");
				if (!lock->Valid()) continue;
				if (!AtomicWriteText(session / "session.json", "{\"format\":\"vega.autosave-session\",\"version\":1}\n", mError)) return false;
				mLock = std::move(lock);
				mRoot = resolver.Root();
				mSession = session;
				mError.clear();
				return true;
			}
			throw std::runtime_error("Cannot reserve an autosave session");
		}
		catch (const std::exception& error) { mError = error.what(); return false; }
	}

	bool AutosaveStore::Write(const std::filesystem::path& source, const std::string& kind, const std::string& text)
	{
		if (!Enabled()) { mError = "Autosave is unavailable for this asset root"; return false; }
		try
		{
			JsonDocument resolver{ mRoot };
			std::filesystem::path absolute;
			if (!SafeSource(resolver, source, absolute)) throw std::runtime_error("Autosave source must stay inside the asset root and outside .vega");
			const auto parsed = Json::parse(text);
			(void)parsed;
			const auto relative = absolute.lexically_relative(mRoot).generic_u8string();
			const auto key = kind + ":" + relative;
			if (!mFiles.count(key)) mFiles[key] = mSession / ("snapshot-" + std::to_string(mFiles.size()) + ".json");
			std::filesystem::path safe;
			if (!resolver.Resolve(mFiles.at(key).lexically_relative(mRoot), safe) || safe != mFiles.at(key)) throw std::runtime_error("Autosave destination changed");
			const Json snapshot{ { "format", "vega.autosave" }, { "version", 1 }, { "source", relative }, { "kind", kind }, { "text", text } };
			return AtomicWriteText(safe, snapshot.dump(1, '\t') + "\n", mError);
		}
		catch (const std::exception& error) { mError = error.what(); return false; }
	}

	std::vector<AutosaveEntry> AutosaveStore::Entries(bool uncleanOnly)
	{
		std::vector<AutosaveEntry> result;
		if (!Enabled()) return result;
		try
		{
			JsonDocument resolver{ mRoot };
			for (const auto& session : std::filesystem::directory_iterator{ mSession.parent_path() })
			{
				if (session.is_symlink() || !session.is_directory()) continue;
				std::filesystem::path safe;
				if (!resolver.Resolve(session.path().lexically_relative(mRoot), safe) || safe != session.path()) continue;
				if (!std::filesystem::exists(safe / "session.json")) continue;
				std::unique_ptr<Lock> lock;
				if (safe != mSession)
				{
					lock = std::make_unique<Lock>(safe / "session.lock");
					if (!lock->Valid()) continue;
				}
				const bool unclean = safe != mSession && !std::filesystem::exists(safe / "clean.json");
				if (uncleanOnly && !unclean) continue;
				for (const auto& file : std::filesystem::directory_iterator{ safe })
				{
					if (file.is_symlink() || !file.is_regular_file() || file.path().filename().u8string().find("snapshot-") != 0 || file.path().extension() != ".json") continue;
					try
					{
						const auto snapshot = Json::parse(Read(file.path()));
						if (snapshot.value("format", std::string{}) != "vega.autosave" || snapshot.at("version") != 1) continue;
						const auto relative = std::filesystem::u8path(snapshot.at("source").get<std::string>());
						std::filesystem::path source;
						if (relative.is_absolute() || !SafeSource(resolver, relative, source)) continue;
						const auto text = snapshot.at("text").get<std::string>();
						const auto parsed = Json::parse(text);
						// Already saved snapshots remain available manually, but need no crash prompt.
						if (uncleanOnly)
						{
							if (snapshot.value("acknowledged", false)) continue;
							try { if (parsed == Json::parse(Read(source))) continue; } catch (const std::exception&) {}
						}
						result.push_back({ file.path(), relative, snapshot.at("kind").get<std::string>(), text, Summary(source, text), unclean });
					}
					catch (const std::exception&) { /* A damaged snapshot must not hide the remaining recovery files. */ }
				}
			}
			std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) { return a.snapshot.generic_u8string() > b.snapshot.generic_u8string(); });
			mError.clear();
		}
		catch (const std::exception& error) { mError = error.what(); }
		return result;
	}

	bool AutosaveStore::Discard(const AutosaveEntry& entry)
	{
		// Only remove an entry found by our own validated enumeration.
		for (const auto& known : Entries()) if (known.snapshot == entry.snapshot)
		{
			std::error_code error;
			std::filesystem::remove(known.snapshot, error);
			mError = error ? error.message() : std::string{};
			return !error;
		}
		mError = "Autosave no longer exists";
		return false;
	}

	bool AutosaveStore::Acknowledge(const AutosaveEntry& entry)
	{
		for (const auto& known : Entries()) if (known.snapshot == entry.snapshot)
		{
			try
			{
				auto snapshot = Json::parse(Read(known.snapshot));
				snapshot["acknowledged"] = true;
				return AtomicWriteText(known.snapshot, snapshot.dump(1, '\t') + "\n", mError);
			}
			catch (const std::exception& error) { mError = error.what(); return false; }
		}
		mError = "Autosave no longer exists";
		return false;
	}
}
