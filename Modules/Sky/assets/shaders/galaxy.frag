#version 120

// Milky Way band rendered once per location into a cache (output size plus a parallax margin).
// Broad starlight and dust shapes carry the colour; sparse resolved stars keep fine detail behind combat:
// - unresolved starlight in two colour families: warm gold/cream old stars along the spine and the bulge, cool blue
//   young-star clouds on the flanks (exaggerated colour separation is the band's style);
// - sparse resolved stars whose density follows the band light;
// - dust lanes: a continuous, frayed optical depth near the spine (no thresholds); extinction is stronger for blue
//   light, so thin dust reddens what is behind it to orange-brown before thick dust darkens it;
// - emission clouds: billowing clouds that hug the plane (where HII regions really are), denser and whiter in their hearts;
// - H-alpha knots: small pink star-forming regions along the band, more of them on spiral arms.
// The output is light in units of uMaxBrightness (background tier cap), black where there is none; it is drawn additively.
uniform vec2 uSize;
uniform vec2 uCenter;
uniform float uAngle;
uniform float uWidth;
uniform float uCenterSide;
uniform float uAntiSide;
uniform float uBulge;
uniform float uArm;
uniform float uDust;
uniform float uStarDensity;
uniform float uSeed;
uniform vec3 uCream;
uniform vec3 uGold;
uniform vec3 uTeal;
uniform vec3 uBlue;
uniform vec3 uPink;
uniform float uMaxBrightness;
// Per-stop style (BandStyle): 0 = built-in look; clouds scales the gas, cloud reach spreads it away from the plane.
uniform float uStyle;
uniform float uClouds;
uniform float uCloudReach;
// The stop's nebulae and haze (DeepSkyLayer), rendered in this cache's pixel frame, in cap units.
uniform sampler2D uExtra;
uniform float uExtraOn;
// Band shape (BandStyle): arc, perspective taper toward one end, and the resolved-star boost.
uniform float uCurve;
uniform float uTaper;
uniform float uStarBoost;
// Low-orbit panorama: the sky as one full turn of the orbit, sampled per texel (0 = the classic screen band).
uniform float uPanorama;        // 0 = the screen band (classic), 1 = a full-turn orbit panorama
uniform float uPixelsPerRadian; // panorama scale
uniform vec3 uPole;             // galactic north pole in the orbit basis
uniform vec3 uGalCenter;        // direction of the galactic centre (l = 0) in the orbit basis
uniform vec3 uGalSide;          // l = +90 deg, = cross(uPole, uGalCenter)

const mat2 kRotate = mat2(0.8, -0.6, 0.6, 0.8);

float hash12(vec2 p)
{
	vec3 p3 = fract(vec3(p.xyx) * 0.1031);
	p3 += dot(p3, p3.yzx + 33.33);
	return fract((p3.x + p3.y) * p3.z);
}

vec2 hash22(vec2 p)
{
	vec3 p3 = fract(vec3(p.xyx) * vec3(0.1031, 0.1030, 0.0973));
	p3 += dot(p3, p3.yzx + 33.33);
	return fract((p3.xx + p3.yz) * p3.zy);
}

// Gradient noise with a quintic fade, roughly in [-0.7, 0.7].
float gnoise(vec2 p)
{
	vec2 i = floor(p);
	vec2 f = fract(p);
	vec2 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);
	float a = dot(hash22(i) * 2.0 - 1.0, f);
	float b = dot(hash22(i + vec2(1.0, 0.0)) * 2.0 - 1.0, f - vec2(1.0, 0.0));
	float c = dot(hash22(i + vec2(0.0, 1.0)) * 2.0 - 1.0, f - vec2(0.0, 1.0));
	float d = dot(hash22(i + vec2(1.0, 1.0)) * 2.0 - 1.0, f - vec2(1.0, 1.0));
	return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float fbm(vec2 p)
{
	float sum = 0.0;
	float amp = 0.5;
	for (int i = 0; i < 5; i++)
	{
		sum += amp * gnoise(p);
		p = kRotate * p * 2.03 + vec2(17.1, 9.3);
		amp *= 0.5;
	}
	return sum;
}

// Soft stars on a grid of `cell` pixels; each cell holds at most one star at a random spot, and the 3x3 neighbourhood is
// evaluated so a star is never cut by its cell border.
vec3 softStars(vec2 px, float cell, float chance, float seed)
{
	vec3 sum = vec3(0.0);
	vec2 base = floor(px / cell);
	for (int j = -1; j <= 1; j++)
	{
		for (int i = -1; i <= 1; i++)
		{
			vec2 c = base + vec2(float(i), float(j));
			float roll = hash12(c + seed);
			if (roll > chance)
			{
				continue;
			}
			vec2 at = (c + 0.15 + 0.7 * hash22(c + seed * 1.7)) * cell;
			float bright = pow(hash12(c * 1.31 + seed), 2.5);
			float radius = 0.7 + 1.1 * bright;
			float d = length(px - at) / radius;
			float shape = exp(-d * d * 1.6);
			// star colour: most stars cream, some blue (young), some orange (giants)
			float t = hash12(c * 2.07 + seed);
			vec3 colour = t < 0.3 ? uBlue : (t > 0.75 ? uGold : uCream);
			sum += colour * shape * (0.35 + 1.4 * bright);
		}
	}
	return sum;
}

// Pink H-alpha knots on a coarse grid (3x3 neighbourhood, radius below half a cell, so never clipped).
float knots(vec2 px, float cell, float chance, float seed)
{
	float sum = 0.0;
	vec2 base = floor(px / cell);
	for (int j = -1; j <= 1; j++)
	{
		for (int i = -1; i <= 1; i++)
		{
			vec2 c = base + vec2(float(i), float(j));
			if (hash12(c + seed) > chance)
			{
				continue;
			}
			vec2 at = (c + 0.3 + 0.4 * hash22(c + seed * 2.3)) * cell;
			float radius = cell * (0.08 + 0.14 * hash12(c * 1.9 + seed));
			vec2 d = (px - at) / radius;
			// clumpy, not a perfect disc
			float clump = 0.6 + 0.8 * (gnoise(px * 0.18 + c * 3.1) + 0.5);
			sum += exp(-dot(d, d) * 1.3) * clump;
		}
	}
	return sum;
}

// Billow: |noise| octaves, puffy rounded lobes at every scale (the cauliflower edges of molecular clouds).
float billow(vec2 p)
{
	float sum = 0.0;
	float amp = 0.5;
	for (int i = 0; i < 5; i++)
	{
		sum += amp * abs(gnoise(p)) * 1.6;
		p = kRotate * p * 2.07 + vec2(3.7, 11.1);
		amp *= 0.52;
	}
	return sum;
}

// The band's cloud complex in band widths (q.y = distance from the spine): billowing clouds that thin out away from the
// plane, so the clouds belong to the galaxy instead of to the screen. Positive inside a cloud.
float cloudField(vec2 q)
{
	vec2 cq = vec2(q.x * 0.5, q.y * 0.9) + vec2(uSeed * 5.3, uSeed * 2.1);
	vec2 warp = vec2(fbm(cq * 0.55 + 5.0), fbm(cq * 0.55 + 9.0)) * 0.9;
	float body = fbm(cq + warp) * 0.9 + billow(cq * 1.5 + warp * 1.3) * 0.45;
	float plane = exp(-q.y * q.y * 1.1 / (uCloudReach * uCloudReach));
	return body + plane * 0.85 - 1.04;
}

// The band at band coordinates u (along) and v (across), in pixels. spx is the position for the stars and knots, px the
// texel for the stop's extra texture; along runs from -1 at the anticentre to +1 at the centre.
vec3 shadeBand(float u, float v, vec2 spx, vec2 px, float along)
{
	// Perspective: with a taper the band narrows toward one end, the way the plane looks when seen along its length,
	// and its structure shrinks with it.
	// Smooth (exponential) narrowing, so there is no crease where a linear taper would hit its limit.
	float taperT = u / (uSize.x * 0.5);
	float w = uWidth * exp(clamp(uTaper * 2.0 * taperT, -3.0, 2.0));
	// Along the band the structure keeps the base scale (scaling it with the local width aliased into streaks at the
	// narrow end); only the cross-section narrows.
	float wBase = uWidth;

	// An arc (the band seen across a wide field) and a little meander.
	float vv = v - fbm(vec2(u / (wBase * 3.5), uSeed * 0.37)) * w * 0.6 + (u * u) / (uSize.x * 2.6) * uCurve;
	float an = vv / w;
	float core = exp(-an * an * 1.1);
	float halo = exp(-an * an * 0.3);

	// Brighter toward the galactic centre (+u), and a bulge swell on that side.
	// The model's band light grows by orders of magnitude toward the centre; on screen it rides on a fixed floor, so the
	// first stops already show a living band and the centre is still the brightest.
	float side = 0.6 + 0.4 * mix(uAntiSide, uCenterSide, smoothstep(-1.0, 1.0, along));
	// Where the band narrows into the distance the line of sight crosses more of the disk: brighter per pixel.
	side *= 1.0 + 1.8 * abs(uTaper) * smoothstep(-0.2, 1.1, -sign(uTaper) * taperT);
	float cloudSide = side;
	// The classic band's bulge sits a little in from the left; in the panorama the galactic centre is the origin of u.
	float bulgeShift = uPanorama > 0.5 ? 0.0 : uSize.x * 0.06;
	vec2 bq = vec2((u - bulgeShift) / (w * 2.8), vv / (w * 1.2));
	float bulge = exp(-dot(bq, bq)) * (0.5 + 0.5 * uBulge) * side;

	vec2 q = vec2(u / wBase, vv / w);
	vec2 sq = vec2(q.x * 0.72, q.y) + vec2(uSeed * 7.3, uSeed * 1.9);

	// Unresolved starlight: star clouds at two scales, bright islands between darker stretches.
	float clouds = smoothstep(-0.3, 0.5, fbm(sq * 0.8) + fbm(sq * 2.6 + 11.0) * 0.4);
	clouds = clouds * clouds * (3.0 - 2.0 * clouds);
	float starlight = (core * 0.8 + halo * 0.14) * side * (0.2 + 0.9 * clouds) + bulge * (0.08 + 1.0 * clouds * clouds);
	// Low-contrast mottling preserves the broad star-cloud shapes without a carpet of sharp grain.
	starlight *= 0.88 + 0.16 * smoothstep(-0.55, 0.55, fbm(sq * 2.0 + 47.0));

	// Emission clouds in the plane: patchy, rose at their thin edges, with soft internal structure.
	float f0 = cloudField(q);
	float cloudA = smoothstep(-0.05, 0.45, f0);
	float cloudDeep = smoothstep(0.15, 0.75, f0);
	float grain = 0.85 + 0.2 * smoothstep(-0.35, 0.35, fbm(vec2(q.x * 2.0, q.y * 3.0) + uSeed * 1.7 + 40.0));
	// Clouds sit a little off the spine's dust, so their rose and teal still read as colour accents in the band.
	float emission = cloudA * (0.3 + 0.55 * cloudDeep) * grain * (0.65 + 0.35 * cloudSide) * uClouds;
	// A few clouds have an [O III] heart (only around the hottest stars): teal inside the rose.
	float oxygen = cloudDeep * smoothstep(0.15, 0.45, fbm(vec2(q.x * 0.5, q.y * 0.8) + vec2(uSeed * 4.1, 77.0)));
	vec3 cloudLight = mix(uPink, uTeal, oxygen * 0.85) * emission;

	// Dust: a continuous optical depth from a many-octave field, never thresholded, so the lanes fray into wisps and
	// thin veils the way real dust does, with stars and glow still showing through the thin parts.
	vec2 dq = vec2(q.x * 0.38, q.y * 1.1) + vec2(uSeed * 3.1, uSeed * 0.7);
	vec2 dwarp = vec2(fbm(dq * 1.1 + 5.0), fbm(dq * 1.1 + 9.0));
	float lane = fbm(dq * 0.9 + dwarp * 0.9) + 0.3 * fbm(dq * 3.1 + dwarp * 1.6 + 20.0);
	float laneOffset = (an + 0.12) / 0.75;
	float spine = exp(-laneOffset * laneOffset);
	float tau = uDust * 2.8 * spine * smoothstep(-0.3, 0.45, lane + spine * 0.15);
	// Filament texture inside the dust: thick and thin threads, so no lane is a flat silhouette.
	tau *= 0.75 + 0.45 * smoothstep(-0.45, 0.45, fbm(vec2(q.x * 1.1, q.y * 2.5) + dwarp * 1.3 + 64.0));
	float wisps = fbm(vec2(q.x * 1.6, q.y * 3.2) + dwarp * 1.6 + 50.0);
	tau += uDust * 0.9 * exp(-an * an * 0.5) * smoothstep(-0.1, 0.45, wisps);
	tau += uDust * 0.4 * halo * smoothstep(0.0, 0.5, fbm(dq * 0.8 + 31.0));
	// Interstellar extinction is stronger for blue light (about 1/wavelength): thin dust turns the light behind it
	// orange-brown before thick dust turns it dark. That gradation is what makes real lanes look natural.
	vec3 trans = exp(-tau * vec3(0.72, 1.0, 1.45));
	float transMean = dot(trans, vec3(0.3333));

	// Colour separation, the band's style: warm old-star light (gold, cream where brightest) along the spine and toward
	// the centre, cool young-star clouds (blue) on the flanks. Broad patches let blue clouds cross the spine and gold
	// spill outward, so the band is a play of warm and cool instead of one hue.
	float patch = smoothstep(-0.35, 0.35, fbm(vec2(q.x * 0.22, q.y * 0.38) + vec2(uSeed * 1.3, 91.0)));
	float warmth = core * 0.75 + bulge * 0.9 + 0.12 * smoothstep(-1.0, 1.0, along);
	warmth = clamp(warmth * mix(0.25, 1.05, patch), 0.0, 1.0);
	// Each family is a light ramp, not a flat paint: dim light is deep and saturated, bright light runs to near white.
	float level = clamp(starlight * 0.8, 0.0, 1.0);
	// Warm family runs coral rose -> amber gold -> cream; cool family runs indigo -> azure -> ice, with teal clouds in it.
	vec3 rose = mix(uPink, uGold, 0.18);
	vec3 warmRamp = level < 0.5 ? mix(rose, uGold, level * 2.0) : mix(uGold, uCream, level * 2.0 - 1.0);
	float tealPatch = smoothstep(0.05, 0.4, fbm(vec2(q.x * 0.4, q.y * 0.7) + vec2(uSeed * 2.3, 123.0)));
	vec3 coolMid = mix(uBlue, uTeal, tealPatch * 0.55);
	vec3 coolRamp = level < 0.5 ? mix(vec3(0.24, 0.26, 0.95), coolMid, level * 2.0) : mix(coolMid, vec3(0.86, 0.95, 1.0), level * 2.0 - 1.0);
	// A wide blend and a faint rose/lavender bridge soften gold-to-blue boundaries without blurring silhouettes.
	float warmWeight = smoothstep(0.08, 0.94, warmth);
	float transition = 4.0 * warmWeight * (1.0 - warmWeight);
	vec3 bridge = mix(mix(uBlue, uPink, 0.32), uCream, level * 0.5);
	vec3 colour = mix(mix(coolRamp, warmRamp, warmWeight), bridge, transition * 0.14);
	// Many HII regions lie on the near side of the dust, so the clouds are dimmed less than the starlight behind it.
	vec3 cloudTrans = mix(trans, sqrt(trans), 0.6);
	vec3 glow = colour * starlight * trans + cloudLight * 1.4 * cloudTrans;
	if (uExtraOn > 0.5)
	{
		// Render textures are stored bottom-up. The stop's nebulae sit in the plane like the band's own clouds, so the
		// same dust dims them; away from the band (no dust) they shine unchanged.
		vec3 extra = texture2D(uExtra, vec2(px.x / uSize.x, 1.0 - px.y / uSize.y)).rgb / max(uMaxBrightness, 1e-4);
		glow += extra * cloudTrans;
	}

	// Resolved stars: density follows the band light, dimmed by the dust like everything behind it.
	float density = clamp(starlight + emission, 0.0, 2.0);
	vec3 stars = softStars(spx, 22.0, uStarDensity * uStarBoost * (0.01 + 0.1 * density), uSeed + 3.0);
	stars += softStars(spx, 48.0, uStarDensity * uStarBoost * (0.02 + 0.08 * density), uSeed + 9.0) * 0.7;
	if (uStarBoost > 1.5)
	{
		// Deep in a star cloud: a third, finer layer of faint stars fills the band.
		stars += softStars(spx, 9.0, uStarDensity * (uStarBoost - 1.0) * 0.06 * density, uSeed + 21.0) * 0.6;
	}
	stars *= trans * 0.6 + 0.4 * sqrt(trans);

	// H-alpha knots: small star-forming regions in the clouds and the core, more on spiral arms.
	float knotChance = (0.05 + 0.1 * uArm) * clamp(cloudA * 0.6 + core * 0.6, 0.0, 1.0);
	vec3 pink = uPink * knots(spx, 100.0, knotChance * 0.6, uSeed + 17.0) * 0.55 * (0.35 + 0.65 * transMean);

	// Aura: the band's scattered light washes far into the sky as a very broad, low-frequency colour field, cool on the
	// anticentre side and warm toward the centre, so the empty sky is deep colour instead of flat navy. No local detail,
	// so it adds colour without pulling the eye from combat.
	// Broad dark rifts keep deep black gaps in it, so it never reads as fog over the whole screen.
	float rift = smoothstep(-0.25, 0.35, fbm(vec2(q.x * 0.14, q.y * 0.22) + vec2(uSeed * 1.7, 157.0)));
	float aura = exp(-an * an * 0.07) * (0.55 + 0.45 * side) * (0.2 + 0.8 * rift);
	float auraWarm = smoothstep(-0.8, 1.0, along) * 0.6 + bulge * 0.4;
	float auraPatch = smoothstep(-0.5, 0.5, fbm(vec2(q.x * 0.08, q.y * 0.12) + vec2(uSeed * 0.9, 211.0)));
	vec3 auraCool = mix(mix(vec3(0.12, 0.2, 0.75), vec3(0.1, 0.45, 0.6), auraPatch), mix(uBlue * 0.6, uTeal * 0.5, auraPatch), uStyle);
	vec3 auraColour = mix(auraCool, mix(uPink, uGold, 0.25), auraWarm * 0.7);
	glow += auraColour * aura * 0.16 * trans;

	// The bulge already brightens the band toward the centre; the extra lift is for the outer stops, so the climax glow
	// behind the fight stays readable.
	// Deep in a star cloud the resolved stars also stand out more against the glow.
	float starGain = 0.4 * (1.0 + 0.2 * max(uStarBoost - 1.0, 0.0));
	vec3 c = (glow * mix(1.35, 1.0, uBulge) + stars * starGain + pink * 0.5) * uMaxBrightness;
	return c;
}

void main()
{
	vec2 px = gl_TexCoord[0].xy;
	vec3 c;
	if (uPanorama > 0.5)
	{
		// One full turn of the orbit across the texture height; the screen x is the angle across the orbit.
		float lambda = 6.28318531 * px.y / uSize.y;
		float phi = (px.x - 0.5 * uSize.x) / uPixelsPerRadian;
		vec3 d = vec3(sin(phi), cos(phi) * cos(lambda), cos(phi) * sin(lambda));
		// Galactic latitude and longitude of the view direction, so the band lies on the sky rather than on the screen.
		float b = asin(clamp(dot(d, uPole), -1.0, 1.0));
		float l = atan(dot(d, uGalSide), dot(d, uGalCenter));
		float u = l * uPixelsPerRadian;
		float v = b * uPixelsPerRadian;
		float along = cos(l);
		c = shadeBand(u, v, vec2(u * cos(b), v), px, along);
		// u jumps by a full turn at the anticentre (l = +-pi); cross-fade the two sides so the seam is continuous.
		float seam = 0.3;
		float edge = abs(l) - (3.14159265 - seam);
		if (edge > 0.0)
		{
			float shifted = u - sign(l) * 6.28318531 * uPixelsPerRadian;
			vec3 c2 = shadeBand(shifted, v, vec2(shifted * cos(b), v), px, along);
			c = mix(c, c2, 0.5 * smoothstep(0.0, seam, edge));
		}
	}
	else
	{
		vec2 dir = vec2(cos(uAngle), sin(uAngle));
		vec2 nrm = vec2(-dir.y, dir.x);
		vec2 rel = px - uCenter;
		float u = dot(rel, dir);
		float v = dot(rel, nrm);
		float along = clamp(u / (uSize.x * 0.55), -1.0, 1.0);
		c = shadeBand(u, v, px, px, along);
	}

	// Soft shoulder into the cap so the brightest core keeps its hue instead of clipping.
	float peak = max(c.r, max(c.g, c.b));
	float knee = 0.55 * uMaxBrightness;
	if (peak > knee)
	{
		float room = uMaxBrightness - knee;
		c *= (knee + room * (1.0 - exp(-(peak - knee) / room))) / peak;
	}
	gl_FragColor = vec4(c, 1.0);
}
