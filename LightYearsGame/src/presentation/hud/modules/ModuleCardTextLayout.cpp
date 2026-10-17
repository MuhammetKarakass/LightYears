#include "presentation/hud/modules/ModuleCardTextLayout.h"
#include "framework/AssetManager.h"
#include <SFML/Graphics/Text.hpp>
#include <algorithm>
#include <vector>

namespace vega::ModuleCardText
{
	namespace
	{
		// Used only when the card font cannot be loaded: breaks on an estimated character budget so the screen
		// still shows readable text instead of failing during a UI refresh.
		Layout ApproximateWrap(const std::string& text, unsigned int characterSize, float width, unsigned int maxLines)
		{
			const std::size_t perLine = std::max<std::size_t>(1, static_cast<std::size_t>(width / (static_cast<float>(characterSize) * 0.6f)));
			const sf::String input = sf::String::fromUtf8(text.begin(), text.end());
			std::vector<sf::String> lines;
			sf::String line, word;
			const auto place = [&]()
			{
				if (word.isEmpty()) return;
				while (word.getSize() > perLine)
				{
					if (!line.isEmpty()) { lines.push_back(line); line.clear(); }
					lines.push_back(word.substring(0, perLine));
					word = word.substring(perLine);
				}
				if (!line.isEmpty() && line.getSize() + 1 + word.getSize() > perLine) { lines.push_back(line); line.clear(); }
				line = line.isEmpty() ? word : line + " " + word;
				word.clear();
			};
			for (const auto codepoint : input)
			{
				if (codepoint == U' ' || codepoint == U'\t' || codepoint == U'\r' || codepoint == U'\n')
				{
					place();
					if (codepoint == U'\n') { lines.push_back(line); line.clear(); }
				}
				else word += sf::String{ codepoint };
			}
			place();
			if (!line.isEmpty()) lines.push_back(line);
			Layout result;
			result.truncated = lines.size() > maxLines;
			if (result.truncated)
			{
				lines.resize(maxLines);
				auto& last = lines.back();
				while (last.getSize() + 3 > perLine && !last.isEmpty()) last.erase(last.getSize() - 1, 1);
				last += "...";
			}
			result.lineCount = static_cast<unsigned int>(lines.size());
			sf::String joined;
			for (const auto& value : lines) { if (!joined.isEmpty()) joined += "\n"; joined += value; }
			const auto utf8 = joined.toUtf8();
			result.text.assign(utf8.begin(), utf8.end());
			return result;
		}
	}

	Layout Wrap(const std::string& text, const std::string& fontPath, unsigned int characterSize, float width, unsigned int maxLines)
	{
		if (text.empty()) return {};
		if (width <= 0.f || maxLines == 0) return { {}, 0, true };
		const auto font = AssetManager::GetAssetManager().LoadFont(fontPath);
		if (!font) return ApproximateWrap(text, characterSize, width, maxLines);
		sf::Text measured{ *font, "", characterSize };
		const auto fits = [&measured, width](const sf::String& line) { measured.setString(line); return measured.getLocalBounds().size.x <= width; };
		const sf::String input = sf::String::fromUtf8(text.begin(), text.end());
		std::vector<sf::String> lines;
		sf::String line, word;
		const auto appendWord = [&]()
		{
			if (word.isEmpty()) return;
			const sf::String candidate = line.isEmpty() ? word : line + " " + word;
			if (fits(candidate)) { line = candidate; word.clear(); return; }
			if (!line.isEmpty()) { lines.push_back(line); line.clear(); }
			for (const auto codepoint : word)
			{
				const sf::String next = line + sf::String{ codepoint };
				if (!line.isEmpty() && !fits(next)) { lines.push_back(line); line.clear(); }
				line += sf::String{ codepoint };
			}
			word.clear();
		};
		for (const auto codepoint : input)
		{
			if (codepoint == U' ' || codepoint == U'\t' || codepoint == U'\r' || codepoint == U'\n')
			{
				appendWord();
				if (codepoint == U'\n') { lines.push_back(line); line.clear(); }
			}
			else word += sf::String{ codepoint };
		}
		appendWord();
		if (!line.isEmpty()) lines.push_back(line);
		Layout result;
		result.truncated = lines.size() > maxLines;
		if (result.truncated)
		{
			lines.resize(maxLines);
			auto& last = lines.back();
			while (!last.isEmpty() && !fits(last + "...")) last.erase(last.getSize() - 1, 1);
			last += "...";
		}
		result.lineCount = static_cast<unsigned int>(lines.size());
		sf::String joined;
		for (const auto& value : lines) { if (!joined.isEmpty()) joined += "\n"; joined += value; }
		const auto utf8 = joined.toUtf8(); result.text.assign(utf8.begin(), utf8.end());
		return result;
	}
}
