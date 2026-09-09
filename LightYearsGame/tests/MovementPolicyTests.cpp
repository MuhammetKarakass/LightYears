#include "gameplay/movement/MovementPolicyController.h"

#include <cmath>
#include <iostream>
#include <limits>

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
	ly::movement::MovementPolicyController controller;
	ly::movement::MovementPolicyRequest zeroDragPolicy;
	zeroDragPolicy.sourceId = "ZeroDrag.Test";
	zeroDragPolicy.speedCapDisabledOverride = true;
	if (!controller.SetPolicy(zeroDragPolicy))
	{
		return Fail("Movement policy was rejected");
	}

	if (!NearlyEqual(controller.ResolveDampingRetention(0.55f), 0.55f) ||
		!std::isinf(controller.ResolveSpeedCap(520.f, 1.55f)))
	{
		return Fail("Zero Drag did not preserve damping while removing the speed cap");
	}

	if (!controller.RemovePolicy(
		"ZeroDrag.Test",
		ly::movement::MovementPolicyReleaseMode::Normalize,
		1300.f,
		1.1f
	))
	{
		return Fail("Movement policy was not released");
	}

	if (!controller.IsNormalizing() ||
		!NearlyEqual(controller.ResolveDampingRetention(0.55f), 0.55f) ||
		!NearlyEqual(controller.ResolveSpeedCap(520.f, 1.f), 1300.f))
	{
		return Fail("Speed-cap normalization did not preserve normal damping");
	}

	controller.Tick(0.55f);
	const float halfWayCap = controller.ResolveSpeedCap(520.f, 1.f);
	if (!(halfWayCap > 520.f && halfWayCap < 1300.f))
	{
		return Fail("Normalization did not reduce the cap progressively");
	}

	controller.Tick(0.55f);
	if (controller.IsNormalizing() ||
		!NearlyEqual(controller.ResolveSpeedCap(520.f, 1.f), 520.f))
	{
		return Fail("Normalization did not finish at the normal cap");
	}

	ly::movement::MovementPolicyRequest sonicWakePolicy;
	sonicWakePolicy.sourceId = "InertialWake.Test";
	sonicWakePolicy.speedCapFlatBonus = 100.f;
	ly::movement::MovementPolicyRequest secondSpeedBonus;
	secondSpeedBonus.sourceId = "OtherMovement.Test";
	secondSpeedBonus.speedCapFlatBonus = 170.f;
	if (!controller.SetPolicy(sonicWakePolicy) || !controller.SetPolicy(secondSpeedBonus) ||
		!NearlyEqual(controller.ResolveSpeedCap(520.f, 1.f), 790.f) ||
		!NearlyEqual(controller.ResolveSpeedCap(520.f, 1.5f), 1185.f))
	{
		return Fail("Flat speed-cap bonuses did not stack additively");
	}

	return 0;
}
