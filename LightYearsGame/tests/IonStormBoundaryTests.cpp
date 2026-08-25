#include "gameplay/ability/ionStorm/IonStormBoundary.h"

#include <cmath>
#include <iostream>

namespace
{
	bool NearlyEqual(float left, float right)
	{
		return std::fabs(left - right) < 0.001f;
	}
}

int main()
{
	const ly::IonStormBoundary boundary = ly::IonStormBoundary::Generate(
		20,
		250.f,
		250.f,
		335.f
	);
	if (boundary.GetControlPointCount() != 20 ||
		!NearlyEqual(boundary.GetInnerCoreRadius(), 250.f) ||
		!NearlyEqual(boundary.GetOuterMaxRadius(), 335.f) ||
		!boundary.Contains({ 0.f, 0.f }) ||
		!boundary.Contains({ 249.f, 0.f }) ||
		boundary.Contains({ 336.f, 0.f }))
	{
		std::cerr << "Ion Storm boundary core or maximum radius contract failed\n";
		return 1;
	}

	const ly::List<sf::Vector2f> firstRender = boundary.BuildBoundaryPoints(64);
	const ly::List<sf::Vector2f> secondRender = boundary.BuildBoundaryPoints(64);
	if (firstRender.size() != 64 || secondRender.size() != 64)
	{
		std::cerr << "Ion Storm boundary render resolution contract failed\n";
		return 1;
	}
	for (std::size_t index = 0; index < firstRender.size(); ++index)
	{
		if (!boundary.Contains(firstRender[index]) ||
			firstRender[index] != secondRender[index])
		{
			std::cerr << "Ion Storm boundary was not stable between gameplay and render sampling\n";
			return 1;
		}
	}

	return 0;
}
