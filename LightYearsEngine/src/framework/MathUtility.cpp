#include "framework/MathUtility.h"

namespace ly
#define PI 3.1415926
{
	sf::Vector2f RotationToVector(float rotation)
	{
		float radians = DegreesToRadians(rotation);
		return sf::Vector2{std::cos(radians),std::sin(radians)};
	}
	float DegreesToRadians(float degrees)
	{
		return degrees * (PI / 180.0f);
	}
	float RadiansToDegrees(float radians)
	{
		return radians * (180.0f / PI);
	}
}