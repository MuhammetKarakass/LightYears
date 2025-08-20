#include "framework/MathUtility.h"
#include <random>

namespace ly
{
	const float PI = 3.1415926535;

	sf::Vector2f RotationToVector(float rotation)
	{
		float radians = DegreesToRadians(rotation);
		return sf::Vector2f{std::cos(radians),std::sin(radians)};
	}
	float DegreesToRadians(float degrees)
	{
		return degrees * (PI / 180.0f);
	}
	float RadiansToDegrees(float radians)
	{
		return radians * (180.0f / PI);
	}
	float LerpFloat(float a, float b, float alpha)
	{
		if (alpha > 1) alpha = 1;
		if (alpha < 0) alpha = 0;
		return a + (b - a) * alpha;
	}
	sf::Color LerpColor(const sf::Color& a, const sf::Color& b, float alpha)
	{
		int LerpR = LerpFloat(a.r,b.r,alpha);
		int LerpG = LerpFloat(a.g,b.g,alpha);
		int LerpB = LerpFloat(a.b,b.b,alpha);
		int LerpA = LerpFloat(a.a,b.a,alpha);

		return sf::Color(LerpR,LerpG,LerpB,LerpA);
	}
	
	sf::Vector2f RandomUnitVector()
	{
		float randomX = RandRange(-1.0f, 1.0f);
		float randomY = RandRange(-1.0f, 1.0f);

		sf::Vector2f randVec{ randomX, randomY };

		NormalizeVector(randVec);

		return randVec;
	}
	/*int RandRange(int Max, int Min)
	{
		std::random
		return 0;
	}*/
	sf::Vector2f LerpVector(const sf::Vector2f a, const sf::Vector2f& b, float alpha)
	{
		float LerpX = LerpFloat(a.x,b.x,alpha);
		float LerpY = LerpFloat(a.y,b.y,alpha);

		return sf::Vector2f(LerpX,LerpY);
	}
}