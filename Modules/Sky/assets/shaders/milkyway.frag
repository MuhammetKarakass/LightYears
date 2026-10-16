#version 120

// Milky Way band seen from inside the disk, like a long exposure of the galactic plane: warm star clouds in the clumps,
// bluer and lacier toward the band edges and in the arms, a ridged dust lane with a reddened rim, sparse pink H-alpha
// knots and a luma-preserving saturation boost. The colour carries the brightness
// (fraction of uMaxBrightness); alpha is coverage for alpha blending.
uniform vec2 uViewSize;
uniform vec2 uScroll;
uniform float uAngle;
uniform float uOffset;
uniform float uWidth;
uniform float uSeed;
uniform float uCenterSide;
uniform float uAntiSide;
uniform float uDustLane;
uniform float uSkyGlow;
uniform float uArmProximity;
uniform vec3 uDiskColor;   // warm light of the star clouds
uniform vec3 uBlueColor;   // young blue light, band edges and arms
uniform vec3 uGlowColor;
uniform vec3 uHAlphaColor; // pink H-alpha knots
uniform float uMaxBrightness;

const vec3 kLuma = vec3(0.2126, 0.7152, 0.0722);
const float kSaturation = 1.3;
// Structure scale: the star clouds and dust lanes are 1.6x larger than the old frequencies, so the galaxy reads as close.
const float kCloseness = 1.6;

float hash(vec2 p)
{
	p = fract(p * vec2(123.34, 456.21));
	p += dot(p, p + 45.32);
	return fract(p.x * p.y);
}

float valueNoise(vec2 p)
{
	vec2 i = floor(p);
	vec2 f = fract(p);
	f = f * f * (3.0 - 2.0 * f);
	return mix(mix(hash(i), hash(i + vec2(1.0, 0.0)), f.x),
	           mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), f.x), f.y);
}

float fbm(vec2 p)
{
	float sum = 0.0;
	float amp = 0.5;
	for (int i = 0; i < 5; i++)
	{
		sum += amp * valueNoise(p);
		p = p * 2.03 + vec2(17.1, 9.2);
		amp *= 0.5;
	}
	return sum;
}

// 1 on thin branching lines where the noise crosses its midpoint, 0 elsewhere.
float ridged(vec2 p)
{
	return 1.0 - abs(2.0 * fbm(p) - 1.0);
}

// Sparse soft spots: one candidate per unit cell, present in about one cell in sixteen. Every pixel sums the knots of its
// 3x3 neighbourhood, so a knot is never cut by the cell border. Radius stays at or below 0.45 cell, so a knot never
// reaches past its immediate neighbours and the 3x3 sum is exact.
float knots(vec2 p)
{
	vec2 cell = floor(p);
	float sum = 0.0;
	for (int j = -1; j <= 1; j++)
	{
		for (int i = -1; i <= 1; i++)
		{
			vec2 c = cell + vec2(float(i), float(j));
			if (hash(c) > 0.0625)
			{
				continue;
			}
			vec2 centre = vec2(hash(c + vec2(3.1, 7.3)), hash(c + vec2(5.9, 1.7)));
			float radius = min(0.2 + 0.25 * hash(c + vec2(11.3, 2.9)), 0.45);
			float k = 1.0 - smoothstep(0.0, radius, length(p - c - centre));
			sum += k * k;
		}
	}
	return min(sum, 1.0);
}

void main()
{
	vec2 dir = vec2(cos(uAngle), sin(uAngle));
	vec2 normal = vec2(-dir.y, dir.x);
	vec2 p = gl_TexCoord[0].xy + uScroll;
	vec2 rel = p - (uViewSize * 0.5 + normal * uOffset);
	float u = dot(rel, dir);
	float v = dot(rel, normal);

	float profile = exp(-(v * v) / (uWidth * uWidth));
	// 0 on the band centre line, 1 at its edge
	float edge = smoothstep(0.15, 1.0, abs(v) / uWidth);

	// star clouds on three scales: big clouds, clumps inside them and a fine grain of stars
	vec2 q = p * (6.0 / (kCloseness * uViewSize.x)) + vec2(uSeed * 37.1, uSeed * 11.7);
	float large = fbm(q);
	float clump = fbm(q * 3.1 + vec2(5.7, 1.3));
	float fine = valueNoise(q * 9.0 + vec2(2.4, 8.1));
	// contrast curve: the cut rises toward the edges, so the edges break up into lace and gaps
	float n = 0.6 * large + 0.4 * clump;
	float cut = mix(0.36, 0.52, edge);
	float cloud = smoothstep(cut, cut + 0.36, n);
	float clouds = mix(0.12, 1.35, cloud) * mix(0.85, 1.1, fine);

	float side = mix(uAntiSide, uCenterSide, smoothstep(-0.6, 0.6, u / uViewSize.x));

	// central bulge swell on the galactic-centre side (+u): a soft elliptical glow, 1.8 band widths across and 2.5 along,
	// centred 0.3 view widths toward the centre. It carries warm light and about half the band's strength.
	const float kCentreU = 0.3;
	const float kBulgeAcross = 1.8;
	const float kBulgeAlong = 2.5;
	const float kBulgeStrength = 0.5;
	float bu = (u - kCentreU * uViewSize.x) / (kBulgeAlong * uWidth);
	float bv = v / (kBulgeAcross * uWidth);
	float bulge = exp(-(bu * bu + bv * bv));

	// dust lane: broad absorbing patches with branching ridged filaments inside them, along the band
	vec2 lane = vec2(u * (2.5 / (kCloseness * uViewSize.x)), v * (8.0 / (kCloseness * uViewSize.x)))
	          + vec2(uSeed * 5.3, uSeed * 2.9);
	float patch = smoothstep(0.35, 0.7, fbm(lane));
	// crisp filaments: a narrow ridge threshold gives sharp edges instead of a soft pow() falloff
	float threads = smoothstep(0.78, 0.96, ridged(lane * 2.3 + vec2(7.1, 3.7)));
	float laneShade = exp(-(v * v) / (0.12 * uWidth * uWidth));
	float absorbed = clamp(uDustLane * laneShade * patch * (0.4 + 0.9 * threads), 0.0, 0.95);
	float dust = 1.0 - absorbed;
	// partly absorbed light is reddened: a reddish-brown rim around the dark core
	float rim = 4.0 * absorbed * (1.0 - absorbed);
	vec3 reddening = mix(vec3(1.0), vec3(0.9, 0.55, 0.35), rim * 0.8);

	// warm light in the bright clumps, blue at the edges, in the arms and where the noise is thin
	float blueMix = clamp(0.2 + 0.5 * edge + 0.25 * (1.0 - cloud) + 0.35 * uArmProximity + 0.25 * (fine - 0.5), 0.0, 1.0);
	vec3 starColor = mix(uDiskColor, uBlueColor, blueMix);

	float band = profile * clouds * side * dust;
	float swell = bulge * kBulgeStrength * mix(0.45, 1.0, cloud) * dust;
	vec3 col = (starColor * band + uDiskColor * swell) * reddening;

	// slow noise breaks the glow up so deep in the bulge it reads as unresolved star clouds, not a flat wash
	float glow = uSkyGlow * mix(0.75, 1.1, fbm(q * 0.5));
	col += uGlowColor * glow;

	// H-alpha knots: sparse pink spots where the arms are
	float ha = 0.0;
	if (uArmProximity * profile > 0.01)
	{
		ha = knots(p * (24.0 / uViewSize.x) + vec2(uSeed * 3.7, uSeed * 1.9)) * profile * uArmProximity * 0.175;
	}
	col += uHAlphaColor * ha;


	// luma-preserving saturation boost
	float luma = dot(col, kLuma);
	vec3 boosted = max(vec3(luma) + (col - vec3(luma)) * kSaturation, vec3(0.0));
	boosted *= luma / max(dot(boosted, kLuma), 1e-5);

	// the brightest channel is capped at uMaxBrightness; alpha keeps the unpremultiplied colour under the cap
	vec3 premult = boosted * uMaxBrightness;
	float peak = max(premult.r, max(premult.g, premult.b));
	float alpha = clamp(1.2 * peak / uMaxBrightness, 0.0, 1.0);
	premult *= min(1.0, uMaxBrightness / max(peak, 1e-5));
	gl_FragColor = vec4(premult / max(alpha, 1e-4), alpha);
}
