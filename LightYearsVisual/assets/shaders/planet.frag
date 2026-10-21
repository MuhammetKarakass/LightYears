#version 120

// Procedural planet: SDF sphere + surface noise quantized to a ramp with hard tone boundaries,
// Bayer dither only near tone boundaries, lit/dark side, outline and rim light.
// Per kind (PlanetKind order): giant bands and a storm spot, lava cracks that glow on the night side, a cloud layer
// turning faster than the surface, and a thin atmosphere glow on the lit limb.
// The host star tints and scales only the lit side; the night side and terminator follow the light direction.
uniform int uSilhouette; // 1 = solid disk occluder, 2 = atmosphere-only additive emitter.
uniform int uScattering;
uniform int uEmitterDisk; // A contained ortho disk can be eclipsed as a whole; a horizon arc cannot.
uniform vec3 uRayleigh;
uniform float uMie;
uniform float uMieG;
uniform float uRayleighHeight;
uniform float uMieHeight;
uniform float uThickness;
uniform float uScatterIntensity;
uniform int uViewSamples;
uniform int uLightSamples;
uniform vec2 uCenter;
uniform float uRadius;
uniform float uPixel;          // output pixels per world pixel
uniform float uSurfacePixel;   // surface-only sample grid, independent of continuous position and limb
uniform vec3 uLight;           // surface -> light, scene axes (x right, y down, z toward viewer)
uniform float uRotation;
uniform float uSeed;
uniform float uDither;         // 0 = hard bands only, 1 = wide dither
uniform float uBrightness;     // scales the final color (1 = ramp as authored)
uniform float uSaturation;     // 1 = ramp as authored, 0 = grayscale
uniform float uOpacity;        // distant background haze; 1 = opaque
uniform vec3 uLightColor;      // host star colour (display space); white = no tint
uniform float uLightIntensity; // strength of the lit side; 1 = as authored
uniform vec3 uRamp[5];
uniform int uKind;             // PlanetKind: 0 Terran .. 3 Lava .. 5 GasGiant, 6 HotJupiter, 7 IceGiant
uniform float uTime;           // seconds: cloud drift and storm swirl
uniform float uCloudAmount;    // 0 = no cloud layer, 1 = dense
uniform vec3 uSpinAxis;        // unit spin axis, scene axes; (0,1,0) turns the surface as it always has
uniform float uHalo;           // atmosphere halo falloff in planet radii; 0.03 = the classic look
uniform float uSurfaceScale;   // frequency of the surface and cloud noise; 1 = as authored
uniform vec3 uAtmosphere;      // limb glow colour (display space), black = none
uniform vec3 uRingNormal;      // unit ring normal, scene axes; the ring shades the surface where the star's light crosses it
uniform float uRingInner;      // ring inner edge, planet radii
uniform float uRingOuter;      // ring outer edge, planet radii; 0 = no ring
uniform float uRingOpacity;    // ring opacity
uniform vec3 uRayCenter;       // ray mode: unit direction from the orbit camera to the planet centre, scene axes
uniform float uRayDistance;    // ray mode: camera distance to the centre in planet radii; 0 = orthographic
uniform vec2 uViewCenter;      // ray mode: screen point of the view centre, px
uniform vec2 uFlow;            // ray mode: unit on-screen direction the sky flows
uniform float uPixelsPerRadian; // ray mode: the sky's pixels per radian
uniform vec3 uOrbitAxis;       // unit axis of the orbit rotation (the sky's orbit normal), scene axes
uniform float uOrbitAngle;     // orbit rotation of the surface about uOrbitAxis, radians; 0 = none

float bayer4(vec2 p)
{
	vec2 q = mod(floor(p), 4.0);
	vec2 lo = mod(q, 2.0);
	vec2 hi = floor(q / 2.0);
	float a = mod(2.0 * lo.x + 3.0 * lo.y, 4.0);
	float b = mod(2.0 * hi.x + 3.0 * hi.y, 4.0);
	return (4.0 * a + b + 0.5) / 16.0;
}

float hash3(vec3 p)
{
	p = fract(p * 0.3183099 + 0.1);
	p *= 17.0;
	return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float valueNoise3(vec3 x)
{
	vec3 i = floor(x);
	vec3 f = fract(x);
	f = f * f * (3.0 - 2.0 * f);
	return mix(mix(mix(hash3(i), hash3(i + vec3(1, 0, 0)), f.x),
	               mix(hash3(i + vec3(0, 1, 0)), hash3(i + vec3(1, 1, 0)), f.x), f.y),
	           mix(mix(hash3(i + vec3(0, 0, 1)), hash3(i + vec3(1, 0, 1)), f.x),
	               mix(hash3(i + vec3(0, 1, 1)), hash3(i + vec3(1, 1, 1)), f.x), f.y), f.z);
}

float fbm3(vec3 p)
{
	float sum = 0.0;
	float amp = 0.5;
	for (int i = 0; i < 4; i++)
	{
		sum += amp * valueNoise3(p);
		p = p * 2.1 + vec3(5.2, 1.3, 8.7);
		amp *= 0.5;
	}
	return sum;
}

vec3 toneColor(int index)
{
	vec3 c = uRamp[0];
	if (index == 1) c = uRamp[1];
	if (index == 2) c = uRamp[2];
	if (index == 3) c = uRamp[3];
	if (index == 4) c = uRamp[4];
	return c;
}

// Band density of a ring at rr (planet radii); the same function as planetring.frag, so the shadow matches the ring.
float ringDensity(float rr, float inner, float outer, float seed)
{
	float t = (rr - inner) / (outer - inner);
	float bands = 0.62 + 0.16 * sin(t * 23.0 + seed) + 0.12 * sin(t * 67.0 + seed * 2.3) + 0.08 * sin(t * 173.0 + seed * 0.7);
	float gap = smoothstep(0.01, 0.03, abs(t - 0.62));
	float edge = smoothstep(0.0, 0.05, t) * (1.0 - smoothstep(0.9, 1.0, t));
	return clamp(bands, 0.0, 1.0) * gap * edge * mix(0.65, 1.0, t);
}

// Turn v about the unit axis k by angle a (Rodrigues). With k = (0,1,0) this is the spin the surface has always had.
vec3 spin(vec3 v, vec3 k, float a)
{
	return v * cos(a) + cross(k, v) * sin(a) + k * dot(k, v) * (1.0 - cos(a));
}

// The ray through this pixel from the orbit camera: the sky's equidistant mapping (OrbitSky::View::Direction).
vec3 pixelRay()
{
	vec2 o = gl_TexCoord[0].xy - uViewCenter;
	float u = dot(o, uFlow) / uPixelsPerRadian;
	float a = dot(o, vec2(-uFlow.y, uFlow.x)) / uPixelsPerRadian;
	return cos(a) * (sin(u) * vec3(uFlow, 0.0) - cos(u) * vec3(0.0, 0.0, 1.0)) + sin(a) * vec3(-uFlow.y, uFlow.x, 0.0);
}

// Distances are in planet radii. Both projections use the same shell and planet shadow.
vec2 sphereInterval(vec3 origin, vec3 dir, float radius)
{
	float b = dot(origin, dir);
	float d = b * b - dot(origin, origin) + radius * radius;
	if (d < 0.0) return vec2(1.0, -1.0);
	return vec2(-b - sqrt(d), -b + sqrt(d));
}

vec2 densityAt(vec3 p)
{
	float altitude = max(length(p) - 1.0, 0.0) / uThickness;
	// A truncated exponential has finite density at the shell boundary. Taper the last fifth to vacuum.
	return exp(-altitude / vec2(uRayleighHeight, uMieHeight)) * (1.0 - smoothstep(0.8, 1.0, altitude));
}

vec3 singleScattering(vec3 origin, vec3 dir, bool mieOnly)
{
	vec2 shell = sphereInterval(origin, dir, 1.0 + uThickness);
	float start = max(shell.x, 0.0);
	float finish = shell.y;
	vec2 ground = sphereInterval(origin, dir, 1.0);
	if (ground.x > 0.0 && ground.y >= ground.x) finish = min(finish, ground.x);
	if (finish <= start) return vec3(0.0);
	vec3 light = normalize(uLight);
	float mu = dot(dir, light); // Incoming photons travel along -light; outgoing along -dir.
	float phaseR = 0.0596831 * (1.0 + mu * mu); // 3 / (16 pi)
	float phaseM = 0.0795775 * (1.0 - uMieG * uMieG)
		/ pow(max(1.0 + uMieG * uMieG - 2.0 * uMieG * mu, 0.001), 1.5);
	float stepView = (finish - start) / float(uViewSamples);
	vec2 opticalView = vec2(0.0);
	vec3 sumR = vec3(0.0);
	vec3 sumM = vec3(0.0);
	// Constant upper bounds keep GLSL 1.20 drivers predictable; uniforms can only reduce work.
	for (int i = 0; i < 8; ++i)
	{
		if (i >= uViewSamples) break;
		vec3 sampleAt = origin + dir * (start + (float(i) + 0.5) * stepView);
		vec2 density = densityAt(sampleAt);
		vec2 halfDepth = density * stepView * 0.5;
		opticalView += halfDepth;
		vec2 shadow = sphereInterval(sampleAt, light, 1.0);
		if (!(shadow.x > 0.0 && shadow.y >= shadow.x))
		{
			float stepLight = max(sphereInterval(sampleAt, light, 1.0 + uThickness).y, 0.0) / float(uLightSamples);
			vec2 opticalLight = vec2(0.0);
			for (int j = 0; j < 4; ++j)
			{
				if (j >= uLightSamples) break;
				opticalLight += densityAt(sampleAt + light * ((float(j) + 0.5) * stepLight)) * stepLight;
			}
			vec2 depth = opticalView + opticalLight;
			vec3 transmittance = exp(-uRayleigh * depth.x - vec3(uMie * depth.y));
			sumR += transmittance * density.x * stepView;
			sumM += transmittance * density.y * stepView;
		}
		opticalView += halfDepth;
	}
	vec3 radiance = sumM * uMie * phaseM;
	if (!mieOnly) radiance += sumR * uRayleigh * phaseR;
	return radiance * uLightColor * uLightIntensity * uScatterIntensity;
}

vec3 scatterBands(vec3 radiance, vec2 ditherPixel)
{
	float luma = dot(radiance, vec3(0.299, 0.587, 0.114));
	if (luma <= 0.00001) return vec3(0.0);
	// Five perceptual luminance bands preserve weak rims without changing their spectral hue.
	float threshold = 0.5 + (bayer4(ditherPixel) - 0.5) * uDither;
	float level = clamp(floor(sqrt(clamp(luma, 0.0, 1.0)) * 4.0 + threshold), 0.0, 4.0) / 4.0;
	// Keep the faint tail continuous: rounding it into the first band creates a hard outer contour.
	float banded = luma < 0.0625 ? 0.0625 * smoothstep(0.0, 0.015625, luma) : level * level;
	vec3 color = radiance * (banded / luma);
	return color / max(1.0, max(color.r, max(color.g, color.b)));
}

float emitterPhase(vec3 dir, vec2 limb, bool ray)
{
	// Peak-normalized HG suppresses a displaced copy of a wide horizon under the radial blur.
	// Ortho has parallel view rays; use limb azimuth there, and restore the full ring as the star centres behind it.
	vec3 light = normalize(uLight);
	float mu = ray ? dot(dir, light) : dot(limb, normalize(light.xy + vec2(1e-4)));
	float phase = pow(0.04 / max(1.64 - 1.6 * mu, 0.04), 1.5); // g = 0.8
	if (!ray && uEmitterDisk != 0) phase = mix(phase, 1.0, smoothstep(0.9, 1.0, -light.z));
	return phase;
}

void main()
{
	vec2 p = (gl_TexCoord[0].xy - uCenter) / uRadius;
	float r2 = dot(p, p);
	// Light from behind the planet (uLight.z < 0) makes the atmosphere light up: forward scattering. Normal planets have z > 0.
	float back = pow(clamp(-normalize(uLight).z, 0.0, 1.0), 3.0);
	// Ray mode: the ray meets the unit sphere at centre Pc at t = b - sqrt(disc), b being its depth of closest approach to the centre.
	bool ray = uRayDistance > 0.0;
	vec3 dir = ray ? pixelRay() : vec3(0.0, 0.0, -1.0);
	vec3 Pc = uRayDistance * uRayCenter;
	float b = dot(dir, Pc);
	float disc = b * b - (uRayDistance * uRayDistance - 1.0);
	bool hit = ray ? (disc >= 0.0 && b - sqrt(max(disc, 0.0)) > 0.0) : r2 <= 1.0;
	if (uSilhouette == 1)
	{
		if (!hit) discard;
		gl_FragColor = vec4(1.0);
		return;
	}
	vec2 atmospherePixel = gl_FragCoord.xy;
	if (!ray && uSurfacePixel > 1.0) atmospherePixel = floor((gl_TexCoord[0].xy - uCenter) / uSurfacePixel);
	vec3 scattered = vec3(0.0);
	if (uScattering != 0)
	{
		if (uSilhouette == 2 && hit) discard;
		if (uSilhouette == 2 && dot(dir, normalize(uLight)) <= 0.0) discard;
		vec3 origin = ray ? -Pc : vec3(p, 2.0 + uThickness);
		vec3 radiance = singleScattering(origin, dir, uSilhouette == 2);
		scattered = scatterBands(radiance, atmospherePixel);
		if (uSilhouette == 2)
		{
			vec2 limb = ray ? normalize(b * dir - Pc).xy : normalize(p);
			// Mie already contains the view/light phase; only ortho needs the missing azimuth localization.
			// Apply it after the bands so quantization cannot lift a weak tail into a uniform bright arc.
			if (!ray) scattered *= emitterPhase(dir, limb, false);
		}
		if (!hit || uSilhouette == 2)
		{
			// The visible shell adds within the remaining brightness headroom; the emitter mask is purely additive.
			float alpha = uSilhouette == 2 ? 0.0 : max(scattered.r, max(scattered.g, scattered.b)) * uOpacity;
			gl_FragColor = vec4(scattered * uBrightness * uOpacity, alpha);
			return;
		}
	}
	else if (uSilhouette == 2 && (hit || back <= 0.0)) discard;
	if (!hit)
	{
		if (ray && b <= 0.0)
		{
			discard;
		}
		// Atmosphere halo just outside the limb, brightest on the side facing the star (forward scattering). In ray mode the altitude
		// is taken at the ray's closest approach to the centre, and the limb is the direction of that approach.
		float alt = ray ? sqrt(max(dot(Pc, Pc) - b * b, 0.0)) - 1.0 : sqrt(r2) - 1.0;
		vec2 limb = ray ? normalize(b * dir - Pc).xy : p / sqrt(r2);
		float strength = max(uAtmosphere.r, max(uAtmosphere.g, uAtmosphere.b));
		float haloOuter = uHalo * (0.14 / 0.03);
		if (strength <= 0.0 || alt > haloOuter)
		{
			discard;
		}
		vec2 toLight = normalize(uLight.xy + vec2(1e-4));
		float facing = smoothstep(-0.45, 0.75, dot(limb, toLight));
		facing = mix(facing, 1.0, back);
		float halo = exp(-alt / uHalo) * (1.0 - smoothstep(uHalo * (0.08 / 0.03), haloOuter, alt)) * (0.12 + 0.88 * facing) * (1.0 + 2.5 * back);
		if (uSilhouette == 2)
		{
			// Match the visible halo's clamped coverage before adding colour. An unbounded halo clips
			// RGB channels independently in the mask, turning warm dust yellow/green after composition.
			gl_FragColor = vec4(uAtmosphere * uBrightness * 1.4 * min(halo * 0.85 * uOpacity, 1.0)
				* back * emitterPhase(dir, limb, ray), 0.0);
			return;
		}
		gl_FragColor = vec4(uAtmosphere * uBrightness * 1.4, halo * 0.85 * uOpacity);
		return;
	}
	vec2 ditherPixel = gl_FragCoord.xy;
	vec3 n;
	float limb;
	// Distance from the limb in planet radii, and the width of one pixel there.
	float edge = 1.0 - sqrt(r2);
	float pixelEdge = uPixel / uRadius;
	if (ray)
	{
		n = normalize((b - sqrt(max(disc, 0.0))) * dir - Pc);
		// Inner limb glow: 0 at the centre of the disc, 1 at the limb.
		limb = 1.0 - smoothstep(0.0, 0.3, dot(-dir, n));
		// Along the ray, a pixel spans b / uPixelsPerRadian radii at the limb's distance.
		edge = 1.0 - sqrt(max(dot(Pc, Pc) - b * b, 0.0));
		pixelEdge = b / uPixelsPerRadian;
	}
	else
	{
		vec2 surfaceP = p;
		if (uSurfacePixel > 1.0)
		{
			vec2 gridPixel = floor((gl_TexCoord[0].xy - uCenter) / uSurfacePixel);
			surfaceP = (gridPixel + 0.5) * uSurfacePixel / uRadius;
			surfaceP /= max(1.0, length(surfaceP));
			ditherPixel = gridPixel;
		}
		float z = sqrt(max(0.0, 1.0 - dot(surfaceP, surfaceP)));
		n = vec3(surfaceP, z);
		// Thin limb glow on the lit side only.
		// The inner edge scales with the halo: a wide planet would otherwise get a limb glow far too broad for its radius.
		limb = smoothstep(1.0 - 0.28 * clamp(uHalo / 0.03, 0.0, 1.0), 1.0, length(surfaceP));
	}

	float lambert = dot(n, normalize(uLight));
	float diffuse = lambert > 0.0 ? lambert * uLightIntensity : lambert;
	float shade = clamp(diffuse * 0.85 + 0.22, 0.0, 1.0);
	float night = 1.0 - smoothstep(-0.05, 0.3, lambert);
	// Smooth volumetric light: wrapped diffuse, so the terminator is soft and the night side is truly dark.
	float daylight = smoothstep(-0.12, 0.75, lambert) * uLightIntensity;
	// The ring's shadow: where the ray from the surface toward the star crosses the ring plane inside the ring, the ground is dimmer.
	float ringShade = 0.0;
	float ringDenom = dot(normalize(uLight), uRingNormal);
	if (uRingOuter > 0.0 && abs(ringDenom) > 1e-3)
	{
		float crossing = -dot(n, uRingNormal) / ringDenom;
		if (crossing > 0.0)
		{
			ringShade = ringDensity(length(n + crossing * normalize(uLight)), uRingInner, uRingOuter, uSeed) * uRingOpacity;
		}
	}
	daylight *= 1.0 - 0.8 * ringShade;

	// Surface coordinates live on the sphere, so bands and continents compress toward the limb by themselves. The orbit turns the
	// ground rigidly with the sky; giants spin each latitude at its own speed (differential rotation) only through the planet's own spin.
	vec3 q = spin(n, uOrbitAxis, uOrbitAngle);
	bool giant = uKind >= 5;
	float rowSpeed = 1.0;
	if (giant)
	{
		rowSpeed = (0.75 + 0.5 * (1.0 - dot(q, uSpinAxis) * dot(q, uSpinAxis))) * (0.85 + 0.3 * hash3(vec3(floor(dot(q, uSpinAxis) * 9.0), uSeed, 1.0)));
	}
	vec3 m = spin(q, uSpinAxis, uRotation * rowSpeed);
	float continents = fbm3(m * 3.2 * uSurfaceScale + uSeed);
	float bands = sin(dot(m, uSpinAxis) * 7.0 * uSurfaceScale + continents * 3.5) * 0.5 + 0.5;
	// Rocky planets: continents first; a faint latitude term only hints at climate zones.
	float surface = mix(continents, bands, 0.12);
	if (giant)
	{
		// Flowing bands: latitude stripes warped by turbulence that moves with the spin.
		float turbulence = fbm3(m * 2.4 * uSurfaceScale + uSeed);
		float stripes = sin(dot(m, uSpinAxis) * 11.0 * uSurfaceScale + turbulence * 4.0) * 0.5 + 0.5;
		surface = mix(turbulence, stripes, 0.8);
	}
	if (uKind == 5 || uKind == 6)
	{
		// Great-Red-Spot style oval: drifts slowly in longitude, rings inside turn over time.
		float stormLon = uSeed * 1.7 + uTime * 0.012;
		float stormLat = -0.35;
		vec3 centre = vec3(sin(stormLon) * cos(stormLat), sin(stormLat), cos(stormLon) * cos(stormLat));
		float d = 1.0 - dot(m, centre);
		float swirl = 0.5 + 0.5 * sin(sqrt(max(d, 0.0)) * 46.0 - uTime * 0.9);
		float spot = 1.0 - smoothstep(0.006, 0.016, d);
		surface = mix(surface, 0.08 + 0.3 * swirl, spot);
	}

	// The flat tone bands are the planet's surface (its art style), chosen by the surface alone; the light then
	// shades them smoothly, so a big planet reads as a lit sphere instead of flat posterized patches.
	float value = clamp(0.5 + (surface - 0.5) * (giant ? 1.3 : 1.9), 0.0, 1.0);

	const float tones = 5.0;
	float threshold = 0.5 + (bayer4(ditherPixel) - 0.5) * uDither;
	float level = clamp(floor(value * (tones - 1.0) + threshold), 0.0, tones - 1.0);
	vec3 color = toneColor(int(level));
	color *= 0.05 + 1.05 * daylight;
	// A warm band along the terminator: sunlight crossing the most atmosphere turns red, as at sunset.
	float duskOffset = (lambert - 0.04) / 0.11;
	float dusk = exp(-duskOffset * duskOffset);
	if (uScattering == 0) color += vec3(0.55, 0.22, 0.12) * dusk * 0.22 * max(uAtmosphere.r + uAtmosphere.g + uAtmosphere.b, 0.4);

	// Lava: crust cracks are thin lines of noise; they are bright on the lit side and glow on the night side.
	float crack = 0.0;
	float crackCore = 0.0;
	if (uKind == 3)
	{
		float web = fbm3(m * 4.5 * uSurfaceScale + uSeed * 0.7);
		crack = 1.0 - smoothstep(0.0, 0.035, abs(web - 0.47));
		crackCore = 1.0 - smoothstep(0.0, 0.012, abs(web - 0.47));
		color = mix(color, uRamp[4], crack * (1.0 - night));
	}
	color *= mix(vec3(1.0), uLightColor, clamp(lambert, 0.0, 1.0));

	// Cloud layer: its own noise, turning 1.6x faster than the surface and drifting slowly with time.
	if (uCloudAmount > 0.0)
	{
		vec3 mc = spin(q, uSpinAxis, uRotation * 1.6);
		float cloudNoise = fbm3(mc * 2.6 * uSurfaceScale + vec3(uSeed * 1.3, uTime * 0.02, uSeed * 0.7));
		float edge0 = mix(0.62, 0.44, uCloudAmount);
		float cover = smoothstep(edge0, edge0 + 0.18, cloudNoise);
		float cloudAlpha = min(0.6, cover * uCloudAmount * 1.2);
		vec3 cloudColor = uRamp[4] * (0.04 + 1.0 * daylight) * mix(vec3(1.0), uLightColor, clamp(lambert, 0.0, 1.0));
		color = mix(color, cloudColor, cloudAlpha);
	}

	if (uScattering == 0)
	{
		color +=uAtmosphere * limb * limb * smoothstep(-0.25, 0.5, lambert) * 0.9;
		color += uAtmosphere * pow(limb, 4.0) * back * 1.2;
	}

	if (uKind == 3)
	{
		// Glow strength varies along the cracks; the crust between them keeps a faint warm glow so the night side is not pure black.
		float heat = 0.3 + 0.7 * smoothstep(0.35, 0.65, fbm3(m * 1.3 * uSurfaceScale + uSeed * 2.1));
		color += mix(uRamp[2] * 1.4, uRamp[4], crackCore) * crack * night * 0.9 * heat;
		color += uRamp[1] * 0.12 * night * heat;
	}

	if (edge < pixelEdge && uAtmosphere.r + uAtmosphere.g + uAtmosphere.b <= 0.0)
	{
		color = uRamp[0] * 0.45;
	}
	if (uScattering != 0) color = min(color + scattered, vec3(1.0));
	color = mix(vec3(dot(color, vec3(0.299, 0.587, 0.114))), color, uSaturation);
	float coverage = smoothstep(0.0, pixelEdge, edge);
	gl_FragColor = vec4(color * uBrightness, uOpacity * coverage);
	if (uScattering != 0) gl_FragColor.rgb *= gl_FragColor.a;
}
