#include "editor/JsonDocument.h"
#include <fstream>
#include <iterator>
#include <set>
#include <cmath>
#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

namespace vega
{
	namespace
	{
		void SkipSpace(const std::string& text, std::size_t& cursor)
		{
			while (cursor < text.size() && (text[cursor] == ' ' || text[cursor] == '\t' || text[cursor] == '\r' || text[cursor] == '\n')) ++cursor;
		}

		std::size_t TokenEnd(const std::string& text, std::size_t cursor)
		{
			if (text.at(cursor) == '"')
			{
				++cursor;
				while (cursor < text.size())
				{
					const char c = text[cursor++];
					if (c == '\\') ++cursor;
					else if (c == '"') return cursor;
				}
				throw std::runtime_error("Unterminated string");
			}
			while (cursor < text.size() && std::string(" ,]}\t\r\n").find(text[cursor]) == std::string::npos) ++cursor;
			return cursor;
		}
	}

	JsonDocument::JsonDocument(std::filesystem::path root)
	{
		std::error_code error;
		mRoot = std::filesystem::canonical(root, error);
		if (error) mError = "Cannot resolve asset root: " + error.message();
	}

	JsonStyle JsonDocument::DetectStyle(const std::string& text)
	{
		JsonStyle style;
		style.newline = text.find("\r\n") != std::string::npos ? "\r\n" : "\n";
		style.trailingNewline = !text.empty() && text.back() == '\n';
		for (std::size_t line = text.find('\n'); line != std::string::npos; line = text.find('\n', line + 1))
		{
			std::size_t cursor = line + 1;
			while (cursor < text.size() && text[cursor] == ' ') ++cursor;
			if (cursor > line + 1)
			{
				style.indent = static_cast<int>(cursor - line - 1);
				break;
			}
			if (cursor < text.size() && text[cursor] == '\t')
			{
				style.indent = 1;
				style.indentChar = '\t';
				break;
			}
		}
		return style;
	}

	bool JsonDocument::Resolve(const std::filesystem::path& relative, std::filesystem::path& result) const
	{
		if (mRoot.empty() || relative.is_absolute() || relative.has_root_name()) return false;
		std::error_code error;
		result = std::filesystem::weakly_canonical(mRoot / relative, error);
		if (error) return false;
		auto candidate = result.begin();
		for (const auto& part : mRoot)
		{
			if (candidate == result.end() || *candidate != part) return false;
			++candidate;
		}
		return true;
	}

	std::string JsonDocument::EscapePointer(const std::string& key)
	{
		std::string result;
		for (const char c : key) result += c == '~' ? "~0" : c == '/' ? "~1" : std::string(1, c);
		return result;
	}

	void JsonDocument::ScanLayout(std::size_t& cursor, const std::string& pointer, const Json& value)
	{
		SkipSpace(mLayoutText, cursor);
		if (!value.is_structured())
		{
			const auto end = TokenEnd(mLayoutText, cursor);
			const Json original = Json::parse(mLayoutText.substr(cursor, end - cursor));
			if (original.type() != value.type() || original != value) throw std::runtime_error("Scalar representation mismatch");
			mScalars.push_back({ pointer, cursor, end, original });
			cursor = end;
			return;
		}
		const char closing = value.is_object() ? '}' : ']';
		++cursor;
		SkipSpace(mLayoutText, cursor);
		std::set<std::string> keys;
		std::size_t count = 0;
		while (mLayoutText.at(cursor) != closing)
		{
			std::string key = std::to_string(count);
			if (value.is_object())
			{
				const auto end = TokenEnd(mLayoutText, cursor);
				key = Json::parse(mLayoutText.substr(cursor, end - cursor)).get<std::string>();
				if (!keys.insert(key).second) throw std::runtime_error("Duplicate object keys cannot be edited safely");
				cursor = end;
				SkipSpace(mLayoutText, cursor);
				if (mLayoutText.at(cursor++) != ':') throw std::runtime_error("Invalid object layout");
			}
			ScanLayout(cursor, pointer + "/" + EscapePointer(key), value.is_object() ? value.at(key) : value.at(count));
			++count;
			SkipSpace(mLayoutText, cursor);
			if (mLayoutText.at(cursor) == ',') { ++cursor; SkipSpace(mLayoutText, cursor); }
			else if (mLayoutText.at(cursor) != closing) throw std::runtime_error("Invalid container layout");
		}
		if (count != value.size()) throw std::runtime_error("Container representation mismatch");
		++cursor;
	}

	bool JsonDocument::Open(const std::filesystem::path& relative)
	{
		std::filesystem::path path;
		if (!Resolve(relative, path)) { mError = "Path escapes the asset root"; return false; }
		try
		{
			std::ifstream input{ path, std::ios::binary };
			if (!input) throw std::runtime_error("Cannot open JSON file");
			std::string text{ std::istreambuf_iterator<char>{ input }, {} };
			if (input.bad()) throw std::runtime_error("Cannot read JSON file");
			Json data = Json::parse(text);
			mPath = path;
			mData = std::move(data);
			mLayoutText = mSavedText = std::move(text);
			mStyle = DetectStyle(mLayoutText);
			mScalars.clear();
			mReadOnlyReason.clear();
			mError.clear();
			++mID;
			try
			{
				std::size_t cursor = 0;
				ScanLayout(cursor, "", mData);
				SkipSpace(mLayoutText, cursor);
				if (cursor != mLayoutText.size() || Serialize() != mLayoutText)
					throw std::runtime_error("Serialization does not reproduce the original bytes");
			}
			catch (const std::exception& error)
			{
				mReadOnlyReason = "Read-only: " + std::string{ error.what() };
				mScalars.clear();
			}
			return true;
		}
		catch (const std::exception& error) { mError = error.what(); return false; }
	}

	bool JsonDocument::Revert()
	{
		return IsOpen() && Open(mPath.lexically_relative(mRoot));
	}

	void JsonDocument::Close()
	{
		mPath.clear();
		mData = nullptr;
		mScalars.clear();
		mLayoutText.clear();
		mSavedText.clear();
		mReadOnlyReason.clear();
		mError.clear();
		++mID;
	}

	const Json* JsonDocument::Get(const std::string& pointer) const
	{
		if (!IsOpen()) return nullptr;
		try { return &mData.at(Json::json_pointer{ pointer }); }
		catch (const std::exception&) { return nullptr; }
	}

	bool JsonDocument::Set(const std::string& pointer, const Json& value)
	{
		const auto* before = Get(pointer);
		if (ReadOnly() || !before || before->is_structured() || before->is_null()) return false;
		Json converted = value;
		// JSON positive literals parse as unsigned; preserve the existing integer's storage type.
		if (before->is_number_integer() && value.is_number_integer())
		{
			if (before->is_number_unsigned())
			{
				if (!value.is_number_unsigned() && value.get<std::int64_t>() < 0) return false;
				converted = value.get<std::uint64_t>();
			}
			else
			{
				if (value.is_number_unsigned() && value.get<std::uint64_t>() > static_cast<std::uint64_t>(INT64_MAX)) return false;
				converted = value.get<std::int64_t>();
			}
		}
		if (before->type() != converted.type() || (converted.is_number_float() && !std::isfinite(converted.get<double>()))) return false;
		mData.at(Json::json_pointer{ pointer }) = std::move(converted);
		return true;
	}

	std::string JsonDocument::Serialize() const
	{
		// Scalar-only edits let us retain inline containers, escapes, numeric spellings,
		// indentation, line endings and the trailing newline instead of reformatting a file.
		std::string result;
		std::size_t cursor = 0;
		for (const auto& scalar : mScalars)
		{
			result.append(mLayoutText, cursor, scalar.start - cursor);
			const auto& value = mData.at(Json::json_pointer{ scalar.pointer });
			if (value.type() == scalar.original.type() && value == scalar.original)
				result.append(mLayoutText, scalar.start, scalar.end - scalar.start);
			else result += value.dump();
			cursor = scalar.end;
		}
		result.append(mLayoutText, cursor, std::string::npos);
		return result;
	}

	bool JsonDocument::Save()
	{
		if (!IsOpen() || ReadOnly()) { mError = "No writable JSON document"; return false; }
		std::filesystem::path verified;
		if (!Resolve(mPath.lexically_relative(mRoot), verified) || verified != mPath)
		{
			mError = "Save path escapes or changed within the asset root";
			return false;
		}
		const auto text = Serialize();
		if (!AtomicWriteText(mPath, text, mError)) return false;
		mSavedText = text;
		mError.clear();
		return true;
	}

	bool AtomicWriteText(const std::filesystem::path& path, const std::string& text, std::string& error)
	{
		// Reserve a unique sibling exclusively so another process's temporary file is never overwritten.
		std::filesystem::path temp;
#ifdef _WIN32
		HANDLE file = INVALID_HANDLE_VALUE;
		for (unsigned int attempt = 0; attempt < 100 && file == INVALID_HANDLE_VALUE; ++attempt)
		{
			temp = path;
			temp += ".vega-" + std::to_string(GetCurrentProcessId()) + "-" + std::to_string(attempt) + ".tmp";
			file = CreateFileW(temp.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
		}
		if (file == INVALID_HANDLE_VALUE) { error = "Cannot create temporary save file"; return false; }
		DWORD written = 0;
		const bool wrote = text.size() <= MAXDWORD && WriteFile(file, text.data(), static_cast<DWORD>(text.size()), &written, nullptr) &&
			written == text.size() && FlushFileBuffers(file);
		CloseHandle(file);
		const bool replaced = wrote && (std::filesystem::exists(path)
			? ReplaceFileW(path.c_str(), temp.c_str(), nullptr, 0, nullptr, nullptr)
			: MoveFileExW(temp.c_str(), path.c_str(), MOVEFILE_WRITE_THROUGH));
#else
		temp = path;
		temp += ".vega.tmp";
		if (std::filesystem::exists(temp)) { error = "Temporary save file already exists"; return false; }
		std::ofstream output{ temp, std::ios::binary };
		output << text;
		output.close();
		std::error_code renameError;
		if (output) std::filesystem::rename(temp, path, renameError);
		const bool replaced = output && !renameError;
#endif
		if (!replaced)
		{
			std::error_code cleanup;
			std::filesystem::remove(temp, cleanup);
			error = "Atomic save failed; original file retained";
			return false;
		}
		error.clear();
		return true;
	}
}
