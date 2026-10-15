#pragma once

#include <nlohmann/json.hpp>
#include <filesystem>
#include <cstdint>
#include <string>
#include <vector>

namespace vega
{
	using Json = nlohmann::ordered_json;

	struct JsonStyle
	{
		int indent = 2;
		char indentChar = ' ';
		std::string newline = "\n";
		bool trailingNewline = false;
	};

	class JsonDocument
	{
	public:
		explicit JsonDocument(std::filesystem::path root);
		static JsonStyle DetectStyle(const std::string& text);
		bool Resolve(const std::filesystem::path& relative, std::filesystem::path& result) const;
		bool Open(const std::filesystem::path& relative);
		bool Revert();
		bool Save();
		void Close();
		bool Set(const std::string& pointer, const Json& value);
		const Json* Get(const std::string& pointer) const;
		std::string Serialize() const;
		bool Dirty() const { return IsOpen() && Serialize() != mSavedText; }
		bool IsOpen() const { return !mPath.empty(); }
		bool ReadOnly() const { return !mReadOnlyReason.empty(); }
		const std::string& ReadOnlyReason() const { return mReadOnlyReason; }
		const std::string& Error() const { return mError; }
		const std::filesystem::path& Root() const { return mRoot; }
		const std::filesystem::path& Path() const { return mPath; }
		const Json& Data() const { return mData; }
		const JsonStyle& Style() const { return mStyle; }
		std::uint64_t ID() const { return mID; }
		static std::string EscapePointer(const std::string& key);

	private:
		struct Scalar
		{
			std::string pointer;
			std::size_t start;
			std::size_t end;
			Json original;
		};
		void ScanLayout(std::size_t& cursor, const std::string& pointer, const Json& value);
		std::filesystem::path mRoot;
		std::filesystem::path mPath;
		Json mData;
		JsonStyle mStyle;
		std::string mLayoutText;
		std::string mSavedText;
		std::string mReadOnlyReason;
		std::string mError;
		std::vector<Scalar> mScalars;
		std::uint64_t mID = 0;
	};
}
