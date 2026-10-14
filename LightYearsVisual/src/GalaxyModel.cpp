#include "visual/GalaxyModel.h"

#include <algorithm>
#include <cmath>

namespace ly::visual::galaxy
{
	namespace
	{
		constexpr float Pi = 3.14159265358979f;
		constexpr float MinRadiusKpc = 0.0001f;
		constexpr float MaxStellarDensity = 1e8f;
		// Caps the band integrand so the nuclear cluster does not swamp the line of sight.
		constexpr float MaxBandDensity = 1e5f;

		// Disk: exponential thin and thick populations, thick/thin = 0.12 at the Sun.
		constexpr float ThinLengthKpc = 2.6f;   // thin disk scale length ~2.6 kpc (Bland-Hawthorn & Gerhard 2016)
		constexpr float ThinHeightKpc = 0.3f;
		constexpr float ThickLengthKpc = 2.0f;
		constexpr float ThickHeightKpc = 0.9f;
		constexpr float ThickToThinAtSun = 0.12f;

		// Bulge, bar and nuclear cluster as one power law with a Gaussian cutoff: ~20 at 1 kpc, ~5e6 at 1 pc.
		constexpr float BulgeAmplitude = 26.3f;
		constexpr float BulgeSlope = 1.8f;
		constexpr float BulgeCutoffKpc = 1.9f;
		constexpr float BulgeAxisRatio = 0.6f;  // flattened: r = sqrt(R^2 + (z/0.6)^2)

		constexpr float ArmWidthKpc = 0.35f;
		constexpr float ArmFadeInnerKpc = 2.5f;  // arms fade out inside the bar
		constexpr float ArmFadeOuterKpc = 3.5f;
		constexpr float ArmNearestThreshold = 0.05f;

		constexpr float DustLengthKpc = 3.2f;
		constexpr float DustHeightKpc = 0.1f;
		constexpr float CmzAmplitude = 50.f;     // central molecular zone
		constexpr float CmzLengthKpc = 0.15f;
		constexpr float CmzHeightKpc = 0.03f;

		constexpr float MetallicityGradient = -0.06f;  // dex per kpc
		constexpr float MetallicityCentre = 0.2f;
		constexpr float MetallicityBlendKpc = 3.f;
		constexpr float MetallicityMin = -1.f;
		constexpr float MetallicityMax = 0.4f;

		// Stylized: a real A_V of ~30 mag would hide the centre completely, so the calibration targets a
		// centre optical depth of ~6 instead.
		constexpr float CentreOpticalDepth = 6.f;
		constexpr float BandLengthKpc = 30.f;
		constexpr int BandSteps = 240;
		// Geometric spacing exp(alpha k / N) over the path: first step ~5 pc.
		constexpr float BandSpacingAlpha = 4.8f;
		constexpr int CentreCalibrationSteps = 400;

		struct ArmSpec
		{
			Arm arm;
			float r0Kpc;      // ridge radius at azimuth 0
			float pitchDeg;
			float strength;
		};

		constexpr ArmSpec ArmTable[] = {
			{Arm::Norma3Kpc, 3.8f, 12.f, 0.8f},
			{Arm::ScutumCentaurus, 5.0f, 12.f, 1.0f},
			{Arm::SagittariusCarina, 6.7f, 12.f, 1.0f},
			{Arm::LocalOrion, 8.25f, 10.f, 0.6f},
			{Arm::Perseus, 10.0f, 12.f, 1.0f},
			{Arm::Outer, 13.5f, 12.f, 0.8f},
		};

		struct BandCalibration
		{
			float kappa = 0.f;     // dust-to-extinction factor, so the centre optical depth is CentreOpticalDepth
			float sunMean = 1.f;   // (centre + anti) / 2 at the Sun
		};

		float Clamp01(float x)
		{
			return std::clamp(x, 0.f, 1.f);
		}

		float Smoothstep(float edge0, float edge1, float x)
		{
			const float t = Clamp01((x - edge0) / (edge1 - edge0));
			return t * t * (3.f - 2.f * t);
		}

		float SafeRadius(float radiusKpc)
		{
			return std::max(radiusKpc, MinRadiusKpc);
		}

		float ThinShape(float radius, float absZ)
		{
			return std::exp(-radius / ThinLengthKpc - absZ / ThinHeightKpc);
		}

		float ThickShape(float radius, float absZ)
		{
			return std::exp(-radius / ThickLengthKpc - absZ / ThickHeightKpc);
		}

		// Sun-position values of the two disk shapes; each population is divided by its own Sun value.
		struct DiskReference
		{
			float thin;
			float thick;
		};

		const DiskReference& DiskSunReference()
		{
			static const DiskReference reference = [] {
				const GalacticPosition sun{};
				return DiskReference{
					ThinShape(SunRadiusKpc, std::fabs(sun.heightKpc)),
					ThickShape(SunRadiusKpc, std::fabs(sun.heightKpc))};
			}();
			return reference;
		}

		float DiskPart(const GalacticPosition& position)
		{
			const float radius = SafeRadius(position.radiusKpc);
			const float absZ = std::fabs(position.heightKpc);
			const DiskReference& sun = DiskSunReference();
			const float thin = ThinShape(radius, absZ) / sun.thin;
			const float thick = ThickShape(radius, absZ) / sun.thick;
			return (thin + ThickToThinAtSun * thick) / (1.f + ThickToThinAtSun);
		}

		float BulgePart(const GalacticPosition& position)
		{
			const float radius = SafeRadius(position.radiusKpc);
			const float zScaled = position.heightKpc / BulgeAxisRatio;
			const float r = SafeRadius(std::sqrt(radius * radius + zScaled * zScaled));
			const float x = r / BulgeCutoffKpc;
			return BulgeAmplitude * std::pow(r, -BulgeSlope) * std::exp(-x * x);
		}

		float DustRaw(const GalacticPosition& position, float armProximity)
		{
			const float radius = SafeRadius(position.radiusKpc);
			const float absZ = std::fabs(position.heightKpc);
			const float disk = std::exp(-radius / DustLengthKpc - absZ / DustHeightKpc) * (1.f + armProximity);
			const float cmzR = radius / CmzLengthKpc;
			const float cmz = CmzAmplitude * std::exp(-cmzR * cmzR - absZ / CmzHeightKpc);
			return disk + cmz;
		}

		float SunDustRaw()
		{
			static const float raw = [] {
				const GalacticPosition sun{};
				return DustRaw(sun, ArmProximity(sun));
			}();
			return raw;
		}

		// Trapezoid-rule path lengths: s_k = L (exp(alpha k/N) - 1) / (exp(alpha) - 1), k = 0..N.
		float BandDistance(int k)
		{
			const float norm = std::exp(BandSpacingAlpha) - 1.f;
			return BandLengthKpc * (std::exp(BandSpacingAlpha * static_cast<float>(k) / BandSteps) - 1.f) / norm;
		}

		// Band brightness from origin along the plane: direction -1 looks toward the centre (and out the far side),
		// +1 looks directly away. Integrand = min(density, cap) * exp(-tau), tau accumulates kappa * dust.
		float BandIntegral(const GalacticPosition& origin, float direction, float kappa)
		{
			const float cosA = std::cos(origin.azimuthRad);
			const float sinA = std::sin(origin.azimuthRad);
			const float originX = origin.radiusKpc * cosA;
			const float originY = origin.radiusKpc * sinA;
			const float stepX = direction * cosA;
			const float stepY = direction * sinA;

			float prevDust = DustDensity(origin);
			float prevGlow = std::min(StellarDensity(origin), MaxBandDensity);
			float prevS = 0.f;
			float tau = 0.f;
			float integral = 0.f;
			for (int k = 1; k <= BandSteps; ++k)
			{
				const float s = BandDistance(k);
				const float ds = s - prevS;
				const float x = originX + stepX * s;
				const float y = originY + stepY * s;
				const GalacticPosition sample{std::hypot(x, y), origin.heightKpc, std::atan2(y, x)};

				const float dust = DustDensity(sample);
				tau += kappa * 0.5f * (prevDust + dust) * ds;
				const float glow = std::min(StellarDensity(sample), MaxBandDensity) * std::exp(-tau);
				integral += 0.5f * (prevGlow + glow) * ds;

				prevDust = dust;
				prevGlow = glow;
				prevS = s;
			}
			return integral;
		}

		// Uncalibrated optical depth from the Sun straight to the centre (midpoint rule on a fine uniform grid).
		float CentreOpticalDepthRaw()
		{
			const float ds = SunRadiusKpc / CentreCalibrationSteps;
			float sum = 0.f;
			for (int i = 0; i < CentreCalibrationSteps; ++i)
			{
				const float s = (static_cast<float>(i) + 0.5f) * ds;
				sum += DustDensity(GalacticPosition{SunRadiusKpc - s, SunHeightKpc, 0.f}) * ds;
			}
			return sum;
		}

		BandCalibration BuildBandCalibration()
		{
			BandCalibration calibration;
			const float raw = CentreOpticalDepthRaw();
			calibration.kappa = raw > 0.f ? CentreOpticalDepth / raw : 0.f;

			const GalacticPosition sun{};
			const float centre = BandIntegral(sun, -1.f, calibration.kappa);
			const float anti = BandIntegral(sun, 1.f, calibration.kappa);
			const float mean = 0.5f * (centre + anti);
			calibration.sunMean = mean > 0.f ? mean : 1.f;
			return calibration;
		}

		const BandCalibration& Calibration()
		{
			static const BandCalibration calibration = BuildBandCalibration();
			return calibration;
		}
	}

	float StellarDensity(const GalacticPosition& position)
	{
		return std::min(DiskPart(position) + BulgePart(position), MaxStellarDensity);
	}

	float DustDensity(const GalacticPosition& position)
	{
		return DustRaw(position, ArmProximity(position)) / SunDustRaw();
	}

	float ArmProximity(const GalacticPosition& position, Arm* nearest)
	{
		const float radius = SafeRadius(position.radiusKpc);
		const float fade = Smoothstep(ArmFadeInnerKpc, ArmFadeOuterKpc, radius);

		float best = 0.f;
		Arm bestArm = Arm::None;
		for (const ArmSpec& arm : ArmTable)
		{
			const float pitchRad = arm.pitchDeg * Pi / 180.f;
			const float tanPitch = std::tan(pitchRad);
			const float cosPitch = std::cos(pitchRad);

			// The spiral repeats every 2 pi in azimuth; use the winding whose ridge is radially closest.
			float radialGap = 1e30f;
			for (int winding = -1; winding <= 1; ++winding)
			{
				const float phi = position.azimuthRad + 2.f * Pi * static_cast<float>(winding);
				const float gap = std::fabs(radius - arm.r0Kpc * std::exp(phi * tanPitch));
				radialGap = std::min(radialGap, gap);
			}

			const float perpendicular = radialGap * cosPitch;
			const float x = perpendicular / ArmWidthKpc;
			const float proximity = arm.strength * std::exp(-x * x) * fade;
			if (proximity > best)
			{
				best = proximity;
				bestArm = arm.arm;
			}
		}

		best = Clamp01(best);
		if (nearest)
		{
			*nearest = best >= ArmNearestThreshold ? bestArm : Arm::None;
		}
		return best;
	}

	float Metallicity(const GalacticPosition& position)
	{
		const float radius = SafeRadius(position.radiusKpc);
		const float diskValue = MetallicityGradient * (radius - SunRadiusKpc);
		const float blend = Smoothstep(0.f, MetallicityBlendKpc, radius);
		const float value = MetallicityCentre + (diskValue - MetallicityCentre) * blend;
		return std::clamp(value, MetallicityMin, MetallicityMax);
	}

	SkyState ComputeSky(const GalacticPosition& position)
	{
		SkyState sky;

		const float disk = DiskPart(position);
		const float bulge = BulgePart(position);
		const float total = disk + bulge;
		sky.starDensity = std::min(total, MaxStellarDensity);
		sky.bulgeFraction = total > 0.f ? bulge / total : 0.f;

		sky.armProximity = ArmProximity(position, &sky.nearestArm);
		sky.dustDensity = DustRaw(position, sky.armProximity) / SunDustRaw();
		sky.metallicity = Metallicity(position);

		const float a = sky.armProximity;
		const float b = sky.bulgeFraction;
		sky.stellarMix.hot = (0.06f + 0.15f * a) * (1.f - b);
		sky.stellarMix.white = 0.20f * (1.f - b);
		sky.stellarMix.yellow = 0.30f * (1.f - b) + 0.10f * b;
		sky.stellarMix.red = 0.35f + 0.55f * b;

		const BandCalibration& calibration = Calibration();
		sky.bandCenterSide = BandIntegral(position, -1.f, calibration.kappa) / calibration.sunMean;
		sky.bandAntiSide = BandIntegral(position, 1.f, calibration.kappa) / calibration.sunMean;

		sky.skyGlow = Clamp01(std::log10(std::max(sky.starDensity, 1.f)) / 6.5f);
		return sky;
	}

	GalacticPosition RoutePosition(float progress)
	{
		const float p = Clamp01(progress);
		GalacticPosition position;
		position.radiusKpc = std::max(SunRadiusKpc * (1.f - p) + MinRadiusKpc * p, MinRadiusKpc);
		position.heightKpc = SunHeightKpc * (1.f - p);
		position.azimuthRad = 0.f;
		return position;
	}

	float DistanceToCenterLy(const GalacticPosition& position)
	{
		const float radius = position.radiusKpc;
		const float height = position.heightKpc;
		return std::sqrt(radius * radius + height * height) * LightYearsPerKpc;
	}

	const char* ArmName(Arm arm)
	{
		switch (arm)
		{
		case Arm::Norma3Kpc:
			return "3 kpc Norma Arm";
		case Arm::ScutumCentaurus:
			return "Scutum-Centaurus Arm";
		case Arm::SagittariusCarina:
			return "Sagittarius-Carina Arm";
		case Arm::LocalOrion:
			return "Orion Spur";
		case Arm::Perseus:
			return "Perseus Arm";
		case Arm::Outer:
			return "Outer Arm";
		case Arm::None:
		default:
			return "Interarm";
		}
	}
}
