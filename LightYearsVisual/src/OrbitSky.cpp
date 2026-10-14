#include "visual/OrbitSky.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "visual/UniformWriter.h"

namespace ly::visual
{
	namespace
	{
		constexpr float kPi = 3.14159265f;
		constexpr float kTwoPi = 6.28318530718f;
		constexpr float kDegToRad = 0.01745329252f;
		// Rows per draw when the panorama is baked: one full-height quad would be a single long GPU job.
		constexpr unsigned kStripRows = 1024;

		float Dot(sf::Vector2f a, sf::Vector2f b)
		{
			return a.x * b.x + a.y * b.y;
		}

		// Angle wrapped to (-pi, pi].
		float WrapPi(float x)
		{
			float r = std::fmod(x + kPi, kTwoPi);
			if (r <= 0.f)
			{
				r += kTwoPi;
			}
			return r - kPi;
		}

		// Panorama height for one full turn of sky at the field of view, and the scale that goes with it. MakeView and Bake both
		// use it, so the window and the bake agree on pixels per radian.
		struct PanoramaScale
		{
			unsigned height = 1;
			float pixelsPerRadian = 1.f;
		};

		PanoramaScale PanoramaFor(float fieldOfViewDeg, float outputHeight)
		{
			const float fovRad = std::max(fieldOfViewDeg * kDegToRad, 1e-3f);
			const float turn = std::max(kTwoPi * outputHeight / fovRad, 1.f);
			const unsigned maxSize = sf::Texture::getMaximumSize();
			PanoramaScale scale;
			scale.height = std::clamp(static_cast<unsigned>(std::lround(turn)), 1u, maxSize);
			scale.pixelsPerRadian = static_cast<float>(scale.height) / kTwoPi;
			return scale;
		}

		sf::Vector3f Cross(sf::Vector3f a, sf::Vector3f b)
		{
			return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
		}

		// a * u + b * v
		sf::Vector3f Combine(float a, sf::Vector3f u, float b, sf::Vector3f v)
		{
			return {a * u.x + b * v.x, a * u.y + b * v.y, a * u.z + b * v.z};
		}

		// Distance from the screen centre to the horizon apex along the flow, px.
		float ApexDistance(const OrbitSky::View& view, const OrbitSettings& orbit)
		{
			const sf::Vector2f n = -view.flow;
			const float halfExtent = 0.5f * (std::fabs(n.x) * view.output.x + std::fabs(n.y) * view.output.y);
			return halfExtent - orbit.coverage * view.output.y;
		}

		float Dot3(sf::Vector3f a, sf::Vector3f b)
		{
			return a.x * b.x + a.y * b.y + a.z * b.z;
		}

		// Turn v about the unit axis k by angle a (Rodrigues), the same as spin() in planet.frag.
		sf::Vector3f Spin(sf::Vector3f v, sf::Vector3f k, float a)
		{
			const sf::Vector3f turned = Combine(std::cos(a), v, std::sin(a), Cross(k, v));
			return Combine(1.f, turned, Dot3(k, v) * (1.f - std::cos(a)), k);
		}

		// Unit normal of the ring: its plane is tilted from the orbit plane by tiltDeg about the node, so it opens and closes as we orbit.
		sf::Vector3f RingNormal(const OrbitRing& ring, const OrbitSky::View& view)
		{
			const float tilt = ring.tiltDeg * kDegToRad;
			const float node = WrapPi(view.phaseRad - ring.nodeDeg * kDegToRad);
			const sf::Vector3f side(view.Across().x, view.Across().y, 0.f);
			const sf::Vector3f flow(view.flow.x, view.flow.y, 0.f);
			const sf::Vector3f toward(0.f, 0.f, 1.f);
			return Combine(std::cos(tilt), side, std::sin(tilt), Combine(std::sin(node), flow, -std::cos(node), toward));
		}

		// Atmosphere thickness in planet radii: the authored screen thickness carried out to the limb, where the ray's distance to the
		// centre changes by sqrt(D^2 - 1) radii per radian.
		float HaloWidth(const OrbitSettings& orbit, const OrbitSky::View& view, float distance)
		{
			return std::max(orbit.atmosphere * view.output.y / view.pixelsPerRadian * std::sqrt(distance * distance - 1.f), 1e-4f);
		}

		// Horizon framing: an orthographic sphere whose limb reaches the horizon apex, ApexDistance from the view centre along the flow.
		void PlaceHorizon(PlanetParams& planet, const OrbitSettings& orbit, const OrbitSky::View& view, sf::Vector2f planetDrift)
		{
			const sf::Vector2f n = -view.flow;
			const sf::Vector2f apex = view.output * 0.5f - n * ApexDistance(view, orbit);
			const float r = orbit.radius * view.output.y;
			planet.rayCamera.reset();
			planet.center = apex - n * r + planetDrift;
			planet.radius = r;
			planet.haloWidth = std::max(orbit.atmosphere * view.output.y / r, 1e-4f);
			planet.surfaceScale = orbit.detail;
		}

		// Disk framing: the whole planet centred on screen, its angular radius from diskRadius.
		void PlaceDisk(PlanetParams& planet, const OrbitSettings& orbit, const OrbitSky::View& view, sf::Vector2f viewCenter)
		{
			const float alpha = std::clamp(orbit.diskRadius * view.output.y / view.pixelsPerRadian, 0.01f, 1.4f);
			const float distance = 1.f / std::sin(alpha);
			planet.rayCamera = PlanetRayCamera{sf::Vector3f(0.f, 0.f, -1.f), distance, viewCenter, view.flow, view.pixelsPerRadian};
			planet.center = viewCenter;
			planet.radius = alpha * view.pixelsPerRadian;
			planet.haloWidth = HaloWidth(orbit, view, distance);
			planet.surfaceScale = orbit.detail;
		}

		// Light of the host star on the horizon planet: its sky direction in the orbit's local frame (U toward the apex, F into the screen,
		// side across), turned so the horizon normal becomes U, the limb normal the shader lights.
		sf::Vector3f HorizonLight(const OrbitSettings& orbit, const OrbitSky::View& view, float longitudeRad, float acrossRad)
		{
			const sf::Vector2f s = view.Across();
			// Local frame of the orbit: U the local vertical (toward the apex), F into the screen, side across the orbit.
			const sf::Vector3f U(-view.flow.x, -view.flow.y, 0.f);
			const sf::Vector3f F(0.f, 0.f, -1.f);
			const sf::Vector3f side(s.x, s.y, 0.f);
			// The star in that frame: its dot products with U, F and side (the same direction as OrbitSky::View::Direction).
			const float delta = WrapPi(view.phaseRad - longitudeRad);
			const float xU = -std::sin(delta) * std::cos(acrossRad);
			const float xF = std::cos(delta) * std::cos(acrossRad);
			const float xS = std::sin(acrossRad);
			// Turn the frame so the horizon normal cos(rho)*U + sin(rho)*F becomes U, the limb normal the shader lights.
			const float rho = ApexDistance(view, orbit) / view.pixelsPerRadian;
			const float lU = xU * std::cos(rho) + xF * std::sin(rho);
			const float lF = -xU * std::sin(rho) + xF * std::cos(rho);
			return Combine(1.f, Combine(lU, U, lF, F), xS, side);
		}
	}

	bool OrbitRing::operator==(const OrbitRing& other) const
	{
		return inner == other.inner && outer == other.outer && tiltDeg == other.tiltDeg && nodeDeg == other.nodeDeg && opacity == other.opacity;
	}

	bool OrbitSettings::operator==(const OrbitSettings& other) const
	{
		return planet == other.planet && kind == other.kind && horizonAngleDeg == other.horizonAngleDeg && coverage == other.coverage
			&& radius == other.radius && atmosphere == other.atmosphere && detail == other.detail && brightness == other.brightness
			&& spin == other.spin && fieldOfViewDeg == other.fieldOfViewDeg && degreesPerMinute == other.degreesPerMinute
			&& startDeg == other.startDeg && starDeg == other.starDeg && starAcrossDeg == other.starAcrossDeg
			&& bandTiltDeg == other.bandTiltDeg && bandNodeDeg == other.bandNodeDeg && centerDeg == other.centerDeg
			&& framing == other.framing && diskRadius == other.diskRadius && ring == other.ring && moon == other.moon;
	}

	sf::Vector2f OrbitSky::View::ScreenPoint(float longitudeRad, float acrossRad) const
	{
		const float delta = WrapPi(phaseRad - longitudeRad);
		return output * 0.5f + drift + flow * (delta * pixelsPerRadian) + Across() * (acrossRad * pixelsPerRadian);
	}

	sf::Vector3f OrbitSky::View::Direction(float longitudeRad, float acrossRad) const
	{
		const float delta = WrapPi(phaseRad - longitudeRad);
		// Along the orbit: the view direction (0, 0, -1) turned by delta toward the flow.
		const sf::Vector3f along(std::sin(delta) * flow.x, std::sin(delta) * flow.y, -std::cos(delta));
		const sf::Vector3f side(Across().x, Across().y, 0.f);
		return Combine(std::cos(acrossRad), along, std::sin(acrossRad), side);
	}

	sf::Vector2f OrbitSky::View::StarOffset() const
	{
		return flow * (phaseRad * pixelsPerRadian) + drift;
	}

	OrbitSky::View OrbitSky::MakeView(const OrbitSettings& orbit, sf::Vector2f output, float secondsSinceStart, sf::Vector2f skyDrift)
	{
		const float horizon = orbit.horizonAngleDeg * kDegToRad;
		View view;
		view.output = output;
		// The sky moves toward the planet, so the flow is opposite to the direction of the horizon apex.
		view.flow = -sf::Vector2f(std::cos(horizon), std::sin(horizon));
		view.pixelsPerRadian = PanoramaFor(orbit.fieldOfViewDeg, output.y).pixelsPerRadian;
		view.phaseRad = (orbit.startDeg + orbit.degreesPerMinute * secondsSinceStart / 60.f) * kDegToRad;
		view.drift = skyDrift;
		return view;
	}

	void OrbitSky::PlacePlanet(PlanetParams& planet, const OrbitSettings& orbit, const View& view, sf::Vector2f planetDrift)
	{
		const sf::Vector2f viewCenter = view.output * 0.5f + planetDrift;
		switch (orbit.framing)
		{
		case OrbitFraming::Disk:
			PlaceDisk(planet, orbit, view, viewCenter);
			break;
		case OrbitFraming::Horizon:
		default:
			PlaceHorizon(planet, orbit, view, planetDrift);
			break;
		}
		// The camera's orbit turns the planet rigidly with the sky (its far side moves with the sky, its near face against it); the planet's
		// own spin about its pole then takes back spin times that angle, so the ground runs slower or faster than the sky. A giant's bands
		// share the spin at their own speeds (planet.frag rowSpeed).
		const sf::Vector3f side(view.Across().x, view.Across().y, 0.f);
		planet.orbitAxis = side;
		planet.orbitAngle = view.phaseRad;
		planet.spinAxis = side;
		planet.rotation = -view.phaseRad * orbit.spin;
		if (orbit.ring)
		{
			planet.ring = PlanetRing{RingNormal(*orbit.ring, view), orbit.ring->inner, orbit.ring->outer, orbit.ring->opacity};
			// The ring is fixed to the planet: its normal in the planet's frame is the camera-frame normal carried by the orbit rotation.
			planet.spinAxis = Spin(planet.ring->normal, side, planet.orbitAngle);
		}
	}

	sf::Vector3f OrbitSky::PlanetLight(const OrbitSettings& orbit, const View& view, float longitudeRad, float acrossRad)
	{
		if (orbit.framing == OrbitFraming::Disk)
		{
			return view.Direction(longitudeRad, acrossRad);
		}
		return HorizonLight(orbit, view, longitudeRad, acrossRad);
	}

	float OrbitSky::StarRadius(const View& view, float starRadiusSolar, float distanceAU)
	{
		// One solar radius is 0.00465 AU across the observer's distance.
		return std::atan(starRadiusSolar * 0.00465047f / std::max(distanceAU, 1e-3f)) * view.pixelsPerRadian;
	}

	bool OrbitSky::Bake(sf::Shader& shader, const GalaxyLayer::Params& band, const OrbitSettings& orbit, sf::Vector2u outputSize)
	{
		const float height = static_cast<float>(outputSize.y);
		const PanoramaScale scale = PanoramaFor(orbit.fieldOfViewDeg, height);
		// Wide enough for any flow direction: the window's diagonal, plus the cache margin on both sides.
		const sf::Vector2f output(outputSize);
		const unsigned width = static_cast<unsigned>(std::ceil(std::sqrt(output.x * output.x + output.y * output.y))) + 2 * GalaxyLayer::CacheMargin;
		const sf::Vector2u size(width, scale.height);
		if (mPanorama.getSize() != size && !mPanorama.resize(size))
		{
			mValid = false;
			return false;
		}

		// The galactic plane is tilted by bandTiltDeg about the band's node on the orbit plane; centerDeg slides the galactic centre
		// along the band from that node.
		const float tau = orbit.bandTiltDeg * kDegToRad;
		const float nu = orbit.bandNodeDeg * kDegToRad;
		const float gamma = orbit.centerDeg * kDegToRad;
		const sf::Vector3f nodeDir(0.f, std::cos(nu), std::sin(nu));
		const sf::Vector3f pole(std::cos(tau), -std::sin(tau) * std::sin(nu), std::sin(tau) * std::cos(nu));
		const sf::Vector3f across = Cross(pole, nodeDir);
		const sf::Vector3f galCenter = Combine(std::cos(gamma), nodeDir, std::sin(gamma), across);
		const sf::Vector3f galSide = Cross(pole, galCenter);

		SetBandUniforms(shader, band, sf::Vector2f(size), height);
		{
			UniformWriter uniforms(shader);
			uniforms.Set("uPanorama", 1.f);
			uniforms.Set("uPixelsPerRadian", scale.pixelsPerRadian);
			uniforms.Set("uPole", sf::Glsl::Vec3(pole));
			uniforms.Set("uGalCenter", sf::Glsl::Vec3(galCenter));
			uniforms.Set("uGalSide", sf::Glsl::Vec3(galSide));
			// The whole sky is baked straight: no band curve, no perspective taper and no extra light.
			uniforms.Set("uCurve", 0.f);
			uniforms.Set("uTaper", 0.f);
			uniforms.Set("uExtraOn", 0.f);
		}
		mPanorama.setView(sf::View(sf::FloatRect({0.f, 0.f}, sf::Vector2f(size))));
		mPanorama.clear(sf::Color::Black);
		for (unsigned y0 = 0; y0 < scale.height; y0 += kStripRows)
		{
			const unsigned rows = std::min(kStripRows, scale.height - y0);
			DrawShaderQuad(mPanorama, shader, sf::FloatRect({0.f, static_cast<float>(y0)}, sf::Vector2f(static_cast<float>(size.x), static_cast<float>(rows))), sf::BlendNone);
		}
		mPanorama.display();
		// Vertically periodic: the window wraps across the seam. Smooth filtering keeps the sub-pixel scroll steady.
		mPanorama.setRepeated(true);
		mPanorama.setSmooth(true);

		mValid = true;
		mOutputSize = outputSize;
		mSeed = band.seed;
		mCap = band.maxBrightness;
		mSettings = band.settings;
		mStyle = band.style;
		mFieldOfView = orbit.fieldOfViewDeg;
		mTilt = orbit.bandTiltDeg;
		mNode = orbit.bandNodeDeg;
		mCenter = orbit.centerDeg;
		mShader = &shader;
		return true;
	}

	void OrbitSky::DrawSky(ShaderLibrary& shaders, sf::RenderTarget& target, const GalaxyLayer::Params& band, const OrbitSettings& orbit, const View& view)
	{
		sf::Shader* shader = shaders.Find("galaxy");
		if (shader == nullptr)
		{
			return;
		}
		const sf::Vector2u outputSize = target.getSize();
		const bool stale = !mValid || mOutputSize != outputSize || mSeed != band.seed || mCap != band.maxBrightness || mSettings != band.settings
			|| mStyle != band.style || mFieldOfView != orbit.fieldOfViewDeg || mTilt != orbit.bandTiltDeg || mNode != orbit.bandNodeDeg
			|| mCenter != orbit.centerDeg || mShader != shader;
		if (stale && !Bake(*shader, band, orbit, outputSize))
		{
			return;
		}

		// The window of the panorama that the view shows: rows follow the phase, columns follow the direction across the flow.
		const sf::Vector2f panorama(mPanorama.getSize());
		float base = std::fmod(view.phaseRad * view.pixelsPerRadian, panorama.y);
		if (base < 0.f)
		{
			base += panorama.y;
		}
		const sf::Vector2f across = view.Across();
		const sf::Vector2f corners[4] = {{0.f, 0.f}, {view.output.x, 0.f}, {0.f, view.output.y}, {view.output.x, view.output.y}};
		sf::Vertex v[4];
		for (std::size_t i = 0; i < 4; ++i)
		{
			const sf::Vector2f rel = corners[i] - view.output * 0.5f - view.drift;
			const float along = Dot(rel, view.flow);
			const float side = Dot(rel, across);
			v[i] = sf::Vertex{corners[i], sf::Color::White, {panorama.x * 0.5f + side, base - along}};
		}
		const sf::Vertex triangles[6] = {v[0], v[1], v[2], v[1], v[3], v[2]};
		sf::RenderStates states(sf::BlendAdd);
		states.texture = &mPanorama.getTexture();
		target.draw(triangles, 6, sf::PrimitiveType::Triangles, states);
	}
}
