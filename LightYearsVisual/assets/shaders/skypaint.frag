#version 120

// Luminous nebula volumes in three parallax layers (far, mid, near), lit from the host star: density drives brightness
// continuously (no flat plateaus), the rims that face the star glow, and a broad ridged term breaks the edges. Output is premultiplied (colour * alpha, alpha) and is drawn
// with the premultiplied-over blend into the nebula target.
// Noise budget: 44 perlin evaluations per fragment (far 19, mid 17, near 8). Each layer: fBm centre + 2 gradient samples,
// 2 ridged octaves (4x and 8x, stretched along the warp); the mid layer adds 1 low-frequency sample for the dark rivers. The near layer uses 2 fBm octaves and the mid warp.
uniform vec2 uViewSize;        // target size, output pixels
uniform vec2 uLight;           // host star, output pixels (may be off-screen)
uniform vec2 uScroll0;         // camera motion * far parallax, output pixels
uniform vec2 uScroll1;         // camera motion * mid parallax
uniform vec2 uScroll2;         // camera motion * near parallax
uniform vec3 uShadow;
uniform vec3 uMid;
uniform vec3 uLit;
uniform vec3 uAccent;
uniform float uCoverage;       // 0..1
uniform float uIntensity;
uniform float uCenterCalm;     // 0..1
uniform float uTime;           // seconds
uniform float uSeed;
uniform float uMaxBrightness;

// Raw density is 0.5 + 0.5 * s(fBm / kFbmSigma * 0.9), s a soft sigmoid, so it never clips to a plateau.
const float kFbmSigma = 0.18;
const float kShape = 0.9 / kFbmSigma;   // density slope per unit of fBm
// Normal strength: the outward normal tilts by this much per unit of density gradient (per noise cell).
const float kBump = 0.3;
// Filament contrast on brightness (ridged term in [0,1]).
const float kFilament = 0.25;
// Edge erosion: how much the ridged term pushes the alpha edge in or out.
const float kErode = 0.25;
// Dark rivers: strength with which the low-frequency ridged dust mask is subtracted from the mid layer's alpha.
const float kRiver = 0.5;

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

// Signed gradient (Perlin) noise with a quintic fade, roughly [-1, 1].
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

// Signed fBm of n octaves (n <= 5), rotated per octave so no octave shares the lattice of another.
float fbm(vec2 p, int n)
{
	float sum = 0.0;
	float amp = 0.5;
	for (int i = 0; i < 5; i++)
	{
		if (i >= n)
		{
			break;
		}
		sum += amp * perlin(p);
		p = vec2(0.8 * p.x - 0.6 * p.y, 0.6 * p.x + 0.8 * p.y) * 2.03 + vec2(17.1, 9.2);
		amp *= 0.5;
	}
	return sum;
}

// Domain warp: a low-frequency displacement of the coordinate, 2 perlin evaluations.
vec2 warpAt(vec2 q, vec2 offset)
{
	return vec2(perlin(q * 0.7 + offset), perlin(q * 0.7 + offset + vec2(5.2, 1.3))) * 0.9;
}

// Unit flow direction of a warp vector. The offset keeps it defined where the warp is zero.
vec2 flowOf(vec2 w)
{
	return normalize(w + vec2(0.001, 0.0));
}

// Ridged term in [0,1]: two squared (1 - |noise|) octaves, isotropic. It gently brightens and erodes the cloud edges.
// 2 perlin evaluations.
float ridgeAt(vec2 p, vec2 flow)
{
	// Isotropic: stretching along the warp flow drew field-line streaks that crawled as the layer drifted.
	vec2 s = p;
	// Broad octaves only: finer ones drew thin crawling lines once the layer drifted (and at the half-resolution target).
	float r1 = clamp(1.0 - abs(perlin(s * 1.8)), 0.0, 1.0);
	float r2 = clamp(1.0 - abs(perlin(s * 3.2 + vec2(11.3, 4.7))), 0.0, 1.0);
	return 0.5 * (pow(r1, 2.0) + pow(r2, 2.0));
}

struct Cloud
{
	float d;      // 0..1 density above the coverage floor
	vec2 grad;    // d(raw density) / d(noise coordinate), per cell
};

// Density of one layer at the warped coordinate qw. The coverage floor is only a soft lower bound: d = (raw - floor) /
// (1 - floor), clamped at 0 below the floor, so brightness keeps varying with the fBm above it. Three fBm samples: the
// centre and two finite-difference offsets for the gradient, which gives the normal without a separate height field.
Cloud cloud(vec2 qw, int octaves, float floorT)
{
	const float eps = 0.08;
	float f0 = fbm(qw, octaves);
	float fx = fbm(qw + vec2(eps, 0.0), octaves);
	float fy = fbm(qw + vec2(0.0, eps), octaves);
	vec2 gf = vec2(fx - f0, fy - f0) / eps;
	float a = kShape * f0;
	float raw = 0.5 + 0.5 * a / sqrt(1.0 + a * a);
	Cloud c;
	c.d = clamp((raw - floorT) / (1.0 - floorT), 0.0, 1.0);
	// chain rule: d(raw)/d(fbm) = 0.5 kShape / (1 + a^2)^1.5. The normal uses the raw density so its tilt does not
	// depend on the coverage floor.
	c.grad = gf * (0.5 * kShape / pow(1.0 + a * a, 1.5));
	return c;
}

// Opacity of a layer before its scale: density curve, eroded at the edge by the ridged term so outlines break into wisps.
float coverageAlpha(float d, float ridge)
{
	float edge = smoothstep(0.0, 0.35, d + kErode * (ridge - 0.5));
	// Steep curve: thin parts become see-through instead of a coloured fog, so the masses read as shapes.
	return clamp(pow(d, 1.6) * 1.9, 0.0, 1.0) * edge;
}

// Colour of one layer before its opacity: brightness grows with density, the dense cores shift toward the lit colour,
// the rims that face the star glow, the interior facing the star is lit, the far side falls 20% toward shadow.
vec3 shade(Cloud c, vec3 L, float accent, float glowAt, float ridge, out float diffuse)
{
	// the outward normal points against the density gradient (density rises into the cloud)
	vec3 n = normalize(vec3(-c.grad * kBump, 1.0));
	diffuse = clamp(dot(n, L) * 0.5 + 0.5, 0.0, 1.0);
	float d = c.d;
	// brightness curve: thin parts stay deep shadow, dense cores reach the lit colour
	float b = pow(d, 1.15);

	vec3 col = mix(uShadow, uMid, b) * (0.55 + 0.6 * b);
	col = mix(col, uLit, b);

	// thin lit rim on the masses that face the star: narrow band of density, added on top rather than mixed
	float edgeBand = smoothstep(0.0, 0.08, d) * (1.0 - smoothstep(0.35, 0.65, d));
	float rim = pow(diffuse, 4.0) * edgeBand * 3.6;
	col += uLit * rim;
	col += uLit * 0.35 * diffuse * d;
	col = mix(col, uShadow, 0.2 * (1.0 - diffuse));

	col = mix(col, uAccent * (0.25 + 0.85 * diffuse) * (0.6 + 0.6 * b), accent * 0.8 * smoothstep(0.0, 0.3, d));
	col += uLit * 0.25 * glowAt * d;

	// filaments only brighten: the factor never drops below 0.75 of the base
	col *= mix(1.0, 0.75 + 0.5 * ridge, kFilament);
	return max(col, vec3(0.0));
}

void main()
{
	vec2 frag = gl_TexCoord[0].xy;
	float W = uViewSize.x;
	float H = uViewSize.y;
	vec2 centre = 0.5 * uViewSize;

	vec2 toLight = uLight - frag;
	float dl = length(toLight);
	// light from the host star: in-plane direction toward the star, lifted a little toward the viewer
	vec3 L = normalize(vec3(toLight, 0.25 * H));
	float glowAt = exp(-dl / (0.6 * H));

	// The clouds fill the whole view (an empty middle framed by fog reads as artificial); centre calm only thins them a
	// little where the player fights.
	vec2 fe = (frag - centre) / (0.6 * uViewSize);
	float centreFade = 1.0 - smoothstep(0.0, 0.8, length(fe));
	float calm = 1.0 - 0.35 * uCenterCalm * centreFade;

	// very slow drift, deeper layers a little faster
	vec2 drift = uTime * vec2(0.6, 0.25);
	// Kept small: the hash loses float precision on large coordinates, which showed as crawling grain.
	vec2 seedOffset = fract(vec2(uSeed * 0.3711, uSeed * 0.1173)) * 40.0;
	// coverage is a soft floor under the density: 0 leaves almost nothing, 1 lets nearly all of it through
	float floorT = mix(0.92, 0.1, uCoverage);
	// a little less cloud in the middle when centre calm is on
	floorT = clamp(floorT + 0.12 * uCenterCalm * centreFade, 0.03, 0.97);

	// far: 1.2 screen widths per feature, 5 octaves (20 evaluations)
	vec2 qF = (frag + uScroll0 + drift) / (1.2 * W) + seedOffset;
	vec2 wF = warpAt(qF, vec2(3.1, 7.7));
	vec2 pF = qF + wF;
	Cloud cloudFar = cloud(pF, 5, floorT);
	float ridgeFar = ridgeAt(pF, flowOf(wF));
	// the accent is a second low-frequency field: the far warp component, so it costs nothing extra
	// Two-tone nebula: a separate low-frequency field turns some masses into the accent colour (teal beside pink).
	float accent = smoothstep(-0.05, 0.3, perlin(qF * 0.8 + vec2(41.0, 13.0)));
	float diffuseFar;
	vec3 colFar = shade(cloudFar, L, accent, glowAt, ridgeFar, diffuseFar);
	float aFar = clamp(1.0 * calm * uIntensity * coverageAlpha(cloudFar.d, ridgeFar), 0.0, 1.0);

	// mid: 0.6 screen widths per feature, 4 octaves, centre calm (17 evaluations)
	vec2 qM = (frag + uScroll1 + 2.0 * drift) / (0.6 * W) + seedOffset + vec2(4.1, 9.9);
	vec2 wM = warpAt(qM, vec2(1.7, 3.9));
	vec2 pM = qM + wM;
	Cloud cloudMid = cloud(pM, 4, floorT);
	float ridgeMid = ridgeAt(pM, flowOf(wM));
	float diffuseMid;
	vec3 colMid = shade(cloudMid, L, accent, glowAt, ridgeMid, diffuseMid);
	// dark rivers: a low-frequency ridged mask (thin lines where the noise crosses zero) subtracted from the mid alpha
	float riverNoise = 1.0 - abs(perlin(pM * 0.45 + vec2(13.7, 2.9)));
	float rivers = smoothstep(0.85, 1.0, riverNoise);
	float aMid = clamp(0.85 * calm * uIntensity * coverageAlpha(cloudMid.d, ridgeMid) - kRiver * rivers, 0.0, 1.0);

	// near: 0.35 screen widths per feature, 2 octaves, sparse wisps only: a higher floor, capped below solid, and half the
	// opacity of the old near layer. It takes the mid warp field (scaled) instead of evaluating its own (9 evaluations).
	vec2 qN = (frag + uScroll2 + 3.0 * drift) / (0.35 * W) + seedOffset + vec2(8.3, 2.2);
	vec2 pN = qN + 0.6 * wM;
	float nearFloor = min(floorT + 0.25, 0.85);
	Cloud cloudNear = cloud(pN, 2, nearFloor);
	float ridgeNear = ridgeAt(pN, flowOf(wM));
	float diffuseNear;
	vec3 colNear = shade(cloudNear, L, accent, glowAt, ridgeNear, diffuseNear);
	colNear = mix(uShadow, colNear, smoothstep(0.3, 0.8, diffuseNear));
	float aNear = clamp(0.5 * 0.45 * calm * uIntensity * coverageAlpha(cloudNear.d, ridgeNear), 0.0, 1.0);

	// composite back to front, premultiplied over
	vec3 acc = colFar * aFar;
	float accA = aFar;
	acc = colMid * aMid + acc * (1.0 - aMid);
	accA = aMid + accA * (1.0 - aMid);
	acc = colNear * aNear + acc * (1.0 - aNear);
	accA = aNear + accA * (1.0 - aNear);

	// back to straight colour. The brightest channel is compressed softly toward the cap (hue kept), so bright cores keep
	// their gradient instead of clipping to a flat plateau. The result stays below uMaxBrightness.
	vec3 c = acc / max(accA, 1e-4);
	float peak = max(c.r, max(c.g, c.b));
	float knee = 0.92 * uMaxBrightness;
	float room = max(uMaxBrightness - knee, 1e-4);
	float over = max(peak - knee, 0.0);
	float newPeak = peak - over + room * (1.0 - exp(-over / room));
	c *= newPeak / max(peak, 1e-4);
	gl_FragColor = vec4(c * accA, accA);
}
