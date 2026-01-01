#pragma once
#include <SFML/graphics.hpp>
#include "framework/Core.h"
#include <random>
#include <type_traits>

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
		if (vectorLength == 0.f) 
		{
			vector.x = T{};  // Zero out the vector components
			vector.y = T{};
			return vector;
		}

		ScaleVector(vector, 1.0f / vectorLength);
		return vector;
	}

	float LerpFloat(float a, float b, float alpha);
	sf::Color LerpColor(const sf::Color& a, const sf::Color& b, float alpha);
	sf::Vector2f LerpVector(const sf::Vector2f a, const sf::Vector2f& b, float alpha);
	sf::Vector2f RandomUnitVector();
	sf::Vector2f RandomVector(const sf::Vector2f& a, const sf::Vector2f& b);

	template <typename T>
	T Lerp(const T&a , const T& b, float alpha)
	{
		return a + (b - a) * alpha;
	}

	template<typename T>
	T RandRange(T min, T max)	
	{
		static thread_local std::mt19937 gen{ std::random_device{}() };
		if constexpr (std::is_integral_v<T>)
		{
			std::uniform_int_distribution<T> distribution{ min, max };
			return distribution(gen);
		}
		else
		{
			std::uniform_real_distribution<T> distribution{ min, max };
			return distribution(gen);
		}
	}



	//int RandRange(int Max, int Min);

}