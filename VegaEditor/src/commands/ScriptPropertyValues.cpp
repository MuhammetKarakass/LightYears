#include "ScriptPropertyValues.h"
#include <cmath>
#include <charconv>
#include <locale>
#include <sstream>

namespace vega
{
	bool ParsePropertyValue(
	    const ly::reflect::PropertyInfo& property, const std::vector<std::string>& arguments, bool expect, ly::reflect::PropertyValue& value, float& tolerance)
	{
		using ly::reflect::PropertyKind;
		const std::size_t count = property.kind == PropertyKind::Vector2 ? 2u : property.kind == PropertyKind::Color ? 4u : 1u;
		if (arguments.size() != count && (!expect || arguments.size() != count + 1)) return false;
		const auto real = [&](std::size_t index, float& number)
		{
			std::istringstream stream{arguments[index]};
			stream.imbue(std::locale::classic());
			return (stream >> number) && stream.eof() && std::isfinite(number);
		};
		const auto integer = [&](std::size_t index, int& number)
		{
			const auto& text = arguments[index];
			const auto result = std::from_chars(text.data(), text.data() + text.size(), number);
			return result.ec == std::errc{} && result.ptr == text.data() + text.size();
		};
		if (arguments.size() > count && (!real(count, tolerance) || tolerance < 0.f)) return false;
		switch (property.kind)
		{
		case PropertyKind::Bool:
			if (arguments[0] != "true" && arguments[0] != "false" && arguments[0] != "1" && arguments[0] != "0") return false;
			value = arguments[0] == "true" || arguments[0] == "1";
			return true;
		case PropertyKind::Int:
		case PropertyKind::Flags:
		case PropertyKind::Enum:
		{
			int number = 0;
			if (!integer(0, number)) return false;
			value = number;
			return true;
		}
		case PropertyKind::Float:
		{
			float number = 0.f;
			if (!real(0, number)) return false;
			value = number;
			return true;
		}
		case PropertyKind::Vector2:
		{
			sf::Vector2f vector;
			if (!real(0, vector.x) || !real(1, vector.y)) return false;
			value = vector;
			return true;
		}
		case PropertyKind::Color:
		{
			int channels[4]{};
			for (std::size_t index = 0; index < 4; ++index)
				if (!integer(index, channels[index]) || channels[index] < 0 || channels[index] > 255) return false;
			value = sf::Color{static_cast<std::uint8_t>(channels[0]), static_cast<std::uint8_t>(channels[1]), static_cast<std::uint8_t>(channels[2]),
			    static_cast<std::uint8_t>(channels[3])};
			return true;
		}
		case PropertyKind::String:
			value = arguments[0];
			return true;
		}
		return false;
	}

	bool PropertyEquals(const ly::reflect::PropertyValue& actual, const ly::reflect::PropertyValue& expected, float tolerance)
	{
		if (actual.index() != expected.index()) return false;
		if (const auto* number = std::get_if<float>(&actual)) return std::abs(*number - std::get<float>(expected)) <= tolerance;
		if (const auto* vector = std::get_if<sf::Vector2f>(&actual))
		{
			const auto other = std::get<sf::Vector2f>(expected);
			return std::abs(vector->x - other.x) <= tolerance && std::abs(vector->y - other.y) <= tolerance;
		}
		return actual == expected;
	}
} // namespace vega
