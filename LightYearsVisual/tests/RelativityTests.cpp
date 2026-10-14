#include "visual/Relativity.h"

#include <cmath>
#include <cstddef>
#include <cstdio>

namespace
{
	int gChecks = 0;
	int gFailures = 0;

	void Check(bool condition, const char* expression, const char* file, int line)
	{
		++gChecks;
		if (!condition)
		{
			++gFailures;
			std::printf("%s:%d: CHECK failed: %s\n", file, line, expression);
		}
	}
}

#define CHECK(cond) Check((cond), #cond, __FILE__, __LINE__)

namespace
{
	namespace rel = ly::visual::relativity;

	bool Near(float a, float b)
	{
		return std::fabs(a - b) <= 1e-4f;
	}

	void TestGamma()
	{
		CHECK(Near(rel::Gamma(0.f), 1.f));
		CHECK(Near(rel::Gamma(0.6f), 1.25f));
	}

	void TestAberration()
	{
		const float cosines[] = {-1.f, -0.5f, 0.f, 0.3f, 1.f};
		for (float c : cosines)
		{
			CHECK(Near(rel::AberratedCos(c, 0.f), c));
		}
		CHECK(Near(rel::AberratedCos(0.f, 0.5f), 0.5f));
		const float betas[] = {0.3f, 0.6f, 0.9f};
		for (float b : betas)
		{
			CHECK(Near(rel::AberratedCos(1.f, b), 1.f));
			CHECK(Near(rel::AberratedCos(-1.f, b), -1.f));
		}
	}

	void TestAberrationMonotonic()
	{
		// a perpendicular source (cos = 0) moves forward as the speed rises
		const float betas[] = {0.f, 0.2f, 0.4f, 0.6f, 0.8f};
		float previous = rel::AberratedCos(0.f, betas[0]);
		for (std::size_t i = 1; i < sizeof(betas) / sizeof(betas[0]); ++i)
		{
			const float current = rel::AberratedCos(0.f, betas[i]);
			CHECK(current > previous);
			previous = current;
		}
	}

	void TestDoppler()
	{
		CHECK(Near(rel::DopplerFactor(1.f, 0.6f), 2.f));
		CHECK(Near(rel::DopplerFactor(-1.f, 0.6f), 0.5f));
		// observer-frame form at an aberrated angle: rest cos = 0 -> observed cos = 0.6 -> D = gamma (1 + 0.6 * 0)
		CHECK(Near(rel::DopplerFactor(0.6f, 0.6f), 1.25f));
	}

	void TestDopplerRestFrameAgreement()
	{
		// D(aberrated angle) must equal gamma (1 + beta cos theta) with the rest-frame angle, for any angle
		const float betas[] = {0.2f, 0.6f, 0.9f};
		const float cosines[] = {-0.8f, -0.2f, 0.f, 0.5f, 0.9f};
		for (float b : betas)
		{
			for (float c : cosines)
			{
				const float observed = rel::AberratedCos(c, b);
				CHECK(Near(rel::DopplerFactor(observed, b), rel::Gamma(b) * (1.f + b * c)));
			}
		}
	}

	void TestBeaming()
	{
		CHECK(Near(rel::BeamingMagnitudeDelta(1.f), 0.f));
		CHECK(Near(rel::BeamingMagnitudeDelta(10.f), -10.f));
		CHECK(rel::BeamingMagnitudeDelta(2.f) < 0.f);
	}
}

int main()
{
	TestGamma();
	TestAberration();
	TestAberrationMonotonic();
	TestDoppler();
	TestDopplerRestFrameAgreement();
	TestBeaming();

	std::printf("RelativityTests: %d checks, %d failures\n", gChecks, gFailures);
	return gFailures == 0 ? 0 : 1;
}
