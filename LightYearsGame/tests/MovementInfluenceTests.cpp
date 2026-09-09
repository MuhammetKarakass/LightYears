#include "gameplay/movement/MovementInfluenceController.h"

#include <cmath>
#include <iostream>

namespace
{
	bool NearlyEqual(float left, float right)
	{
		return std::abs(left - right) < 0.0001f;
	}

	int Fail(const char* message)
	{
		std::cerr << message << '\n';
		return 1;
	}
}

int main()
{
	ly::movement::MovementInfluenceController controller;
	controller.ApplyImpulse({ { 100.f, 0.f }, 0.25f });
	const ly::List<sf::Vector2f>& initialOffsets = controller.AdvanceImpulses(1.f);
	if (initialOffsets.size() != 1 || !NearlyEqual(initialOffsets[0].x, 100.f))
	{
		return Fail("Movement impulse did not produce its initial displacement");
	}
	const ly::List<sf::Vector2f>& decayedOffsets = controller.AdvanceImpulses(1.f);
	if (decayedOffsets.size() != 1 || !NearlyEqual(decayedOffsets[0].x, 25.f))
	{
		return Fail("Movement impulse did not decay independently by retention");
	}

	controller.SetAccelerationSource({ 11u, { 40.f, -10.f } });
	controller.SetAccelerationSource({ 12u, { -15.f, 30.f } });
	const sf::Vector2f combinedAcceleration = controller.ResolveAcceleration();
	if (!NearlyEqual(combinedAcceleration.x, 25.f) ||
		!NearlyEqual(combinedAcceleration.y, 20.f))
	{
		return Fail("Continuous movement acceleration sources did not stack");
	}
	controller.RemoveSource(12u);
	if (!NearlyEqual(controller.ResolveAcceleration().x, 40.f) ||
		!NearlyEqual(controller.ResolveAcceleration().y, -10.f))
	{
		return Fail("Removing a movement source removed another source's acceleration");
	}

	controller.SetForcedMovement({ 20u, { 100.f, 0.f }, 1 });
	controller.SetForcedMovement({ 10u, { 50.f, 0.f }, 1 });
	if (!NearlyEqual(controller.ResolveForcedVelocity().x, 50.f))
	{
		return Fail("Forced movement ties were not resolved deterministically by source ID");
	}
	controller.SetForcedMovement({ 30u, { 0.f, -250.f }, 2 });
	const sf::Vector2f forcedVelocity = controller.ResolveForcedVelocity();
	if (!NearlyEqual(forcedVelocity.x, 0.f) || !NearlyEqual(forcedVelocity.y, -250.f))
	{
		return Fail("Higher-priority forced movement did not own translation");
	}

	return 0;
}
