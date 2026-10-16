#pragma once

#include "editor/JsonDocument.h"
#include <memory>

namespace vega
{
	struct AutosaveEntry
	{
		std::filesystem::path snapshot, source;
		std::string kind, text, summary;
		bool unclean = false;
	};

	// All snapshots are private to one session. The OS releases its lock on a
	// crash, so another live editor can never be mistaken for a crashed one.
	class AutosaveStore
	{
	public:
		AutosaveStore();
		~AutosaveStore();
		AutosaveStore(const AutosaveStore&) = delete;
		AutosaveStore& operator=(const AutosaveStore&) = delete;
		bool Begin(const std::filesystem::path& root);
		bool Write(const std::filesystem::path& source, const std::string& kind, const std::string& text);
		std::vector<AutosaveEntry> Entries(bool uncleanOnly = false);
		bool Discard(const AutosaveEntry& entry);
		bool Acknowledge(const AutosaveEntry& entry);
		void SimulateUncleanExit() { mSimulateUnclean = true; }
		bool Enabled() const { return !mSession.empty(); }
		const std::filesystem::path& Session() const { return mSession; }
		const std::string& Error() const { return mError; }
	private:
		struct Lock;
		std::unique_ptr<Lock> mLock;
		std::filesystem::path mRoot, mSession;
		std::map<std::string, std::filesystem::path> mFiles;
		std::string mError;
		bool mSimulateUnclean = false;
	};
}
