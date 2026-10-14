#version 120

// HII region around a hot ionizing star (Stromgren sphere), drawn as a luminous, irregular, elongated gas cloud.
// Line colours are computed on the CPU (Astro WavelengthColor) and weighted by where each ion lives:
//   core rw < 0.6        : [O III] 500.7 nm (needs ~35 eV photons, so only around O-type stars): teal-green, brightest at the cluster
//   everywhere inside    : H-alpha + H-beta (Case B, H-beta = 1/2.86 of H-alpha) forms the glowing body: rose
//   outer half           : [N II] and [S II] rise gradually toward the edge of the gas: deep red
// rw is the warped, rotated, elongated radius of the envelope. The body never goes dark: a low-frequency base glows
// everywhere inside the envelope, ridged filaments add brighter wisps on top, meandering dust lanes and pillars darken
// it, and only the star-facing tips of the pillars are lit (photo-evaporation). Zones are picked by a sharpened weight
// rather than summed, so overlaps do not turn grey.
// Output is premultiplied (colour, max channel); the draw uses additive blending, so emission adds light.
// Everything is zero outside the envelope, and the envelope is zero by |q| = 1.5 (the quad is 1.6 radii).
uniform vec2 uScroll;          // camera motion * parallax, output pixels
uniform vec2 uCenter;          // ionizing star, output pixels
uniform float uRadius;         // Stromgren radius, output pixels
uniform float uSeed;
uniform float uTime;           // seconds, slow drift of the filaments
uniform float uDensity;
uniform float uSaturation;
uniform float uMaxBrightness;
uniform vec3 uHalpha;
uniform vec3 uHbeta;
uniform vec3 uOIII;
uniform vec3 uNII;
uniform vec3 uSII;
uniform vec4 uLineWeights;     // H-alpha, H-beta (1/2.86 of H-alpha), [N II], [S II]
uniform float uOIIIWeight;
uniform float uPillarCount;    // 2..4
uniform vec4 uPillarAngles;    // pillar ray angles, radians; the first uPillarCount are used

float hash(vec2 p)
{
	p = fract(p * vec2(123.34, 456.21));
	p += dot(p, p + 45.32);
	return fract(p.x * p.y);
}

// Unit gradient of a lattice cell, from a hash: a random direction, not a random height.
vec2 gradient(vec2 cell)
{
	float angle = hash(cell) * 6.2831853;
	return vec2(cos(angle), sin(angle));
}

// Signed gradient (Perlin) noise with a quintic fade, roughly [-1, 1]. Value noise has an axis-aligned lattice, which
// shows as straight, square outlines once thresholded.
float perlin(vec2 p)
{
	vec2 i = floor(p);
	vec2 f = fract(p);
	vec2 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);
	float n00 = dot(gradient(i), f);
	float n10 = dot(gradient(i + vec2(1.0, 0.0)), f - vec2(1.0, 0.0));
	float n01 = dot(gradient(i + vec2(0.0, 1.0)), f - vec2(0.0, 1.0));
	float n11 = dot(gradient(i + vec2(1.0, 1.0)), f - vec2(1.0, 1.0));
	return 1.4 * mix(mix(n00, n10, u.x), mix(n01, n11, u.x), u.y);
}

// Signed fBm, 3 octaves, clamped to [-1, 1]: large irregular lumps for the envelope and the body.
float fbm3(vec2 p)
{
	float sum = 0.0;
	float amp = 0.5;
	for (int i = 0; i < 3; i++)
	{
		sum += amp * perlin(p);
		p = vec2(0.8 * p.x - 0.6 * p.y, 0.6 * p.x + 0.8 * p.y) * 2.1 + vec2(11.3, 4.7);
		amp *= 0.5;
	}
	return clamp(sum / 0.875, -1.0, 1.0);
}

// Ridged multifractal, 4 octaves: 1 - |noise| is 1 on the zero contours of the noise, which are curved, filament-like
// lines. Squaring sharpens the crest, and each octave is weighted by the one below, so ridges branch like real gas.
float ridged4(vec2 p)
{
	float sum = 0.0;
	float amp = 0.5;
	float prev = 1.0;
	for (int i = 0; i < 4; i++)
	{
		float n = 1.0 - abs(perlin(p));
		n *= n;
		sum += amp * n * prev;
		prev = n;
		// Rotated and scaled per octave, so no octave shares the lattice of another.
		p = vec2(0.8 * p.x - 0.6 * p.y, 0.6 * p.x + 0.8 * p.y) * 2.03 + vec2(17.1, 9.2);
		amp *= 0.5;
	}
	return sum;
}

// Pillar (elephant trunk) along the ray at angle a: a rounded tip at r = 0.5 inside the body that widens outward and
// fades past the envelope, so it reads as a dark lane cut into the glowing gas. Geometric (no noise).
float column(float r, float theta, float a)
{
	float dth = mod(theta - a + 3.14159265, 6.2831853) - 3.14159265;
	float side = smoothstep(0.0, 0.25, cos(dth));          // only the half-line outward from the centre
	float dp = abs(sin(dth)) * r;                          // distance from the ray, in radii
	float wid = 0.03 + 0.10 * max(r - 0.5, 0.0);
	return exp(-pow(dp / wid, 2.0)) * side * smoothstep(0.5, 0.7, r) * (1.0 - smoothstep(0.95, 1.2, r));
}

float pillarMask(vec2 wp)
{
	float r = length(wp);
	float theta = atan(wp.y, wp.x);
	float pm = 0.0;
	if (uPillarCount > 0.5) pm = max(pm, column(r, theta, uPillarAngles.x));
	if (uPillarCount > 1.5) pm = max(pm, column(r, theta, uPillarAngles.y));
	if (uPillarCount > 2.5) pm = max(pm, column(r, theta, uPillarAngles.z));
	if (uPillarCount > 3.5) pm = max(pm, column(r, theta, uPillarAngles.w));
	return pm;
}

void main()
{
	vec2 p = gl_TexCoord[0].xy + uScroll;
	vec2 q = (p - uCenter) / uRadius;

	// Envelope: a seeded rotation and aspect (0.55-0.85) make the cloud elongated along a random axis; the radius is
	// displaced by a large-scale fbm (+-35%) so the outline is irregular, not a circle. A square window keeps it exactly
	// zero by the quad edge (|q| = 1.6) even where the displacement stretches the envelope.
	float rot = uSeed * 2.39996;
	float aspect = 0.55 + 0.30 * fract(uSeed * 1.6180339 + 0.37);
	vec2 qr = vec2(cos(rot) * q.x + sin(rot) * q.y, -sin(rot) * q.x + cos(rot) * q.y);
	float rw0 = length(qr / vec2(1.0, aspect));
	float shapeN = fbm3(q * 0.9 + uSeed * vec2(21.7, 3.3));
	float rw = rw0 * (1.0 + 0.35 * shapeN);
	float window = 1.0 - smoothstep(1.25, 1.5, max(abs(q.x), abs(q.y)));
	float envelope = (1.0 - smoothstep(0.55, 1.15, rw)) * window;

	// Domain warp, 2 noise evaluations: the ionized gas is clumpy, so the structures are measured from a displaced position.
	vec2 wq = q * 1.7 + uSeed * vec2(13.1, 7.3);
	vec2 warpN = vec2(perlin(wq), perlin(wq + vec2(5.2, 9.7)));
	vec2 w = q + 0.2 * warpN;

	// Luminous body: a low-frequency base that glows everywhere inside the envelope (never a dark interior).
	float lo = 0.5 + 0.5 * fbm3(q * 1.1 + uSeed * vec2(5.9, 17.4) + vec2(3.0, 1.0));
	float base = envelope * mix(0.45, 1.0, lo);

	// Filaments: crisp ridged crests (4 octaves), slowly advected by uTime. They ADD brighter wisps (+60%) on the body.
	float ridge = ridged4(w * 2.6 + uSeed * 3.7 + uTime * vec2(0.004, 0.0025));
	float fil = smoothstep(0.5, 0.75, ridge);
	float wisp = 1.0 + 0.6 * fil;

	// Pillars: dark columns cut into the body, mottled by the warp noise. The mask gradient gives the edge that faces the
	// star; only that tip is lit, and only there.
	float pmGeom = pillarMask(w);
	float e = 0.02;
	vec2 grad = vec2(pillarMask(w + vec2(e, 0.0)) - pmGeom, pillarMask(w + vec2(0.0, e)) - pmGeom) / e;
	float r = length(w);
	vec2 toStar = -w / max(r, 0.001);
	float gLen = length(grad);
	// Cosine between the mask gradient and the direction to the star: 1 where the edge faces the star, 0 on the flanks.
	float facing = max(0.0, -dot(grad, toStar)) / max(gLen, 0.001);
	float rimLight = facing * smoothstep(2.0, 8.0, gLen) * envelope;

	// Two meandering dark dust lanes across the body: soft bands along the zero contours of two noise fields.
	float inBody = smoothstep(0.15, 0.4, rw) * (1.0 - smoothstep(0.85, 1.0, rw));
	float laneA = smoothstep(0.92, 0.99, 1.0 - abs(perlin(w * 1.25 + uSeed * vec2(5.3, 1.9))));
	vec2 wr = vec2(0.6 * w.x - 0.8 * w.y, 0.8 * w.x + 0.6 * w.y);
	float laneB = smoothstep(0.94, 0.99, 1.0 - abs(perlin(wr * 1.7 + uSeed * vec2(2.9, 8.1))));
	float lanes = clamp(0.75 * laneA + 0.5 * laneB, 0.0, 1.0) * inBody;
	float dark = clamp((1.0 - 0.8 * pmGeom * (0.75 + 0.25 * warpN.y)) * (1.0 - 0.7 * lanes), 0.0, 1.0);

	// Ionization zones, in the envelope radius.
	// [O III]: strongest at the cluster, falling off over the inner 0-0.6 of rw.
	float zO = 1.0 - smoothstep(0.0, 0.6, rw);
	// [N II] / [S II]: gradual rise toward the outer half of the gas.
	float outerZ = smoothstep(0.35, 0.95, rw);

	// Line weights in H-alpha luma units: the [O III] unit is scaled so that one unit of it has the luma of one unit of
	// H-alpha, then uOIIIWeight (2.1 at full excitation) sets the core to about 0.6-1.0 of the brightest H-alpha wisp or more.
	float lumaHa = dot(uHalpha, vec3(0.2126, 0.7152, 0.0722));
	float lumaO = max(dot(uOIII, vec3(0.2126, 0.7152, 0.0722)), 0.05);
	float oNorm = lumaHa / lumaO;

	float iO = envelope * zO * (0.6 + 0.4 * lo) * uOIIIWeight * oNorm * wisp * dark;
	float iH = base * wisp * dark;
	float iF = base * (0.35 + 0.65 * outerZ) * dark;

	vec3 cO = uOIII * iO;
	vec3 cH = (uHalpha * uLineWeights.x + uHbeta * uLineWeights.y) * iH;
	// [N II] and [S II] are weak lines, so their zone is scaled up; the pillar rims are scaled further so they read as
	// bright edges, not as a faint tint of the body.
	vec3 cF = (uNII * uLineWeights.z + uSII * uLineWeights.w) * (2.0 * iF + 3.0 * rimLight);

	// Pick, don't sum: the strongest zone keeps its colour (sharpened weights, exponent 3 for soft transitions), so teal
	// and rose meet along a gradual boundary instead of mixing into grey.
	float sO = max(cO.r, max(cO.g, cO.b));
	float sH = max(cH.r, max(cH.g, cH.b));
	float sF = max(cF.r, max(cF.g, cF.b));
	float m = max(sO, max(sH, sF)) + 1e-4;
	float kO = pow(sO / m, 3.0);
	float kH = pow(sH / m, 3.0);
	float kF = pow(sF / m, 3.0);
	vec3 c = (cO * kO + cH * kH + cF * kF) / max(kO + kH + kF, 1e-6);

	// Overall level: the body averages about half the cap, and the brightest wisps reach it through the shoulder below.
	c *= uDensity * uMaxBrightness * 1.6;

	// Chroma boost with luma preserved: the lines get saturated without getting brighter.
	float luma = dot(c, vec3(0.2126, 0.7152, 0.0722));
	c = max(vec3(luma) + (c - vec3(luma)) * uSaturation, vec3(0.0));

	// Soft shoulder: linear below 70% of the cap, then it approaches the cap, so rims reach it without a flat clip.
	float peak = max(c.r, max(c.g, c.b));
	float knee = 0.7 * uMaxBrightness;
	float span = max(uMaxBrightness - knee, 1e-4);
	if (peak > knee)
	{
		float shoulder = knee + span * (1.0 - exp(-(peak - knee) / span));
		c *= shoulder / peak;
	}
	gl_FragColor = vec4(c, max(c.r, max(c.g, c.b)));
}
