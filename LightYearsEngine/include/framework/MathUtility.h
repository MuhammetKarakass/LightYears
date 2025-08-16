#include <SFML/graphics.hpp>
#include "framework/Core.h"

namespace ly
{
	sf::Vector2f RotationToVector(float rotation);

	float DegreesToRadians(float degrees);
	float RadiansToDegrees(float radians);

	template<typename T>
	float GetVectorLength(const sf::Vector2<T>& vector)
	{
		return std::sqrt(vector.x * vector.x + vector.y * vector.y);
	}

	template<typename T>
	sf::Vector2<T>& ScaleVector(sf::Vector2<T>& vectorToScale, float amt)
	{
		vectorToScale.x *= amt;
		vectorToScale.y *= amt;
		return vectorToScale;
	}

	template<typename T>
	sf::Vector2<T>& NormalizeVector(sf::Vector2<T>& vector)
	{
		float vectorLength = GetVectorLength<T>(vector);
		if (vectorLength == 0.f) return sf::Vector2<T>{};

		ScaleVector(vector, 1.0f / vectorLength);

		return vector;
	}
}