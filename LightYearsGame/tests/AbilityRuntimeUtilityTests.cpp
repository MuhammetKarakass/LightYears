#include "gameplay/ability/glacialPressure/GlacialPressurePushMath.h"
#include "gameplay/time/PeriodicTickAccumulator.h"
#include "gameplay/time/IntervalDebt.h"
#include "gameplay/targeting/SweptGeometry.h"

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
	if (!NearlyEqual(
		ly::glacialPressure::ResolvePushDistance(800.f, 1.f, 1.25f),
		1000.f
	))
	{
		return Fail("Glacial Pressure maximum-health scaling was capped at base push distance");
	}
	if (!NearlyEqual(
		ly::glacialPressure::ResolvePushDistance(800.f, 0.4f, 1.25f),
		400.f
	))
	{
		return Fail("Glacial Pressure segment and maximum-health scaling did not compose");
	}

	float accumulator = 0.f;
	if (ly::time::ConsumePeriodicTicks(accumulator, 2.5f, 0.25f) != 4 ||
		!NearlyEqual(accumulator, 1.5f))
	{
		return Fail("Periodic tick scheduler did not cap catch-up work while preserving debt");
	}
	if (ly::time::ConsumePeriodicTicks(accumulator, 0.f, 0.25f) != 4 ||
		!NearlyEqual(accumulator, 0.5f))
	{
		return Fail("Periodic tick scheduler did not preserve deferred ticks");
	}
	if (ly::time::ConsumePeriodicTicks(accumulator, 0.f, 0.25f) != 2 ||
		!NearlyEqual(accumulator, 0.f))
	{
		return Fail("Periodic tick scheduler did not drain deferred ticks exactly");
	}

	float intervalRemaining = 0.05f;
	ly::time::AdvanceIntervalDebt(intervalRemaining, 0.30f);
	if (!NearlyEqual(intervalRemaining, -0.25f))
	{
		return Fail("Interval scheduler discarded fire-rate debt after a long frame");
	}
	int recoveredIntervals = 0;
	while (intervalRemaining <= 0.f &&
		recoveredIntervals < ly::time::DefaultMaximumIntervalCatchUp)
	{
		ly::time::CommitInterval(intervalRemaining, 0.10f);
		++recoveredIntervals;
	}
	if (recoveredIntervals != 3 || !NearlyEqual(intervalRemaining, 0.05f))
	{
		return Fail("Interval scheduler did not recover the exact number of owed shots");
	}

	const sf::FloatRect targetBounds{ { 10.f, 10.f }, { 20.f, 20.f } };
	if (!ly::targeting::swept::SegmentIntersectsExpandedBounds(
		{ 0.f, 20.f }, { 40.f, 20.f }, targetBounds, 0.f
	) || ly::targeting::swept::SegmentIntersectsExpandedBounds(
		{ 0.f, 0.f }, { 5.f, 0.f }, targetBounds, 0.f
	))
	{
		return Fail("Swept target geometry did not classify expanded bounds correctly");
	}
	if (!NearlyEqual(
		ly::targeting::swept::SegmentProjectionFraction(
			{ 20.f, 20.f }, { 0.f, 20.f }, { 40.f, 20.f }
		),
		0.5f
	))
	{
		return Fail("Swept target geometry did not preserve contact ordering");
	}

	return 0;
}
