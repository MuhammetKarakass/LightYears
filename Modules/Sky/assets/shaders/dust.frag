#version 120

// Cold dust cloud, one program with two passes selected by uPass:
//   0: extinction. Transmitted fraction exp(-tau) multiplies what is already drawn (BlendMultiply).
//   1: reflection. Starlight from the illuminator scattered by the grains, added on top (BlendAdd). The sunlit skin of
//      the cloud also gets a faint warm rim, on top of the blue scatter.
// tau: a smooth core times a lumpy fbm, so the cloud is a soft, irregular absorber, not a black blob. The edge is feathered
// over the outer 40% of the ellipse, and the feather line itself is moved by a large-scale noise, so there is no hard
// boundary and no circular outline. A few denser knots absorb more light. Peak transmission is about 0.25 at the default depth.
// The profile reaches exactly zero by 1.045 of the ellipse, inside the quad (1.05).
// Stylize (uStylize = 1): both passes are evaluated at the centre of each style pixel (uPixelSize, layer pixels) and their
// intensity is cut into 4 bands with Bayer dither, so the cloud reads as banded pixel shapes. Each band is mean-preserving.
// Off (uStylize = 0): the same maths at the fragment itself, with smooth values.
uniform float uPass;
uniform float uStylize;        // 0 smooth, 1 banded style pixels
uniform vec2 uScroll;          // camera motion * parallax, output pixels
uniform vec2 uCenter;          // cloud centre, output pixels
uniform vec2 uHalfSize;        // half extents along the cloud axes, output pixels
uniform vec2 uAxis;            // unit vector along the major axis (cos, sin)
uniform float uOpticalDepth;
uniform float uReflection;
uniform vec2 uIlluminator;     // lighting star, output pixels
uniform vec3 uReflectColor;    // scattered-light colour, display space
uniform float uSeed;
uniform float uMaxBrightness;
uniform float uPixelSize;      // style pixel, layer pixels (>= 1)

float bayer4(vec2 p)
{
	vec2 q = mod(floor(p), 4.0);
	vec2 lo = mod(q, 2.0);
	vec2 hi = floor(q / 2.0);
	float a = mod(2.0 * lo.x + 3.0 * lo.y, 4.0);
	float b = mod(2.0 * hi.x + 3.0 * hi.y, 4.0);
	return (4.0 * a + b + 0.5) / 16.0;
}

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

// Signed gradient noise with a quintic fade, roughly [-1, 1] (value noise shows axis-aligned edges).
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

// Signed fBm, 3 octaves, clamped to [-1, 1].
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

void main()
{
	// Style pixel: the cloud is evaluated at the centre of the style pixel under this fragment.
	float px = max(uPixelSize, 1.0);
	vec2 cell = floor(gl_TexCoord[0].xy / px);
	vec2 at = uStylize > 0.5 ? (cell + 0.5) * px : gl_TexCoord[0].xy;
	vec2 p = at + uScroll;
	vec2 perp = vec2(-uAxis.y, uAxis.x);
	vec2 d = p - uCenter;
	// Ellipse coordinates: 0 at the centre, 1 on the boundary.
	vec2 e = vec2(dot(d, uAxis), dot(d, perp)) / uHalfSize;
	float rad = length(e);

	// Irregular outline: a large-scale fbm moves the feather line by up to +-0.08 ellipse radii.
	float shapeN = clamp(fbm3(e * 1.4 + uSeed * vec2(3.1, 7.7)), -0.6, 0.6);
	// Lumps: mid-scale density variation, 0..1.
	float lumps = 0.5 + 0.5 * fbm3(e * 2.4 + uSeed * vec2(9.3, 2.1) + vec2(4.0, 1.0));
	// Feather over the outer 40% of the ellipse.
	float feather = 1.0 - smoothstep(0.6, 1.0, rad + 0.14 * shapeN);
	// Window: exactly zero by 1.045 ellipse radii, so the quad edge (at 1.05 or more) never shows.
	float window = 1.0 - smoothstep(1.02, 1.045, rad);

	// Smooth gaussian core times the feathered lumps (0.6-1.0), plus a few denser knots.
	float core = exp(-2.5 * rad * rad);
	float density = core * feather * window * (0.6 + 0.4 * lumps);
	float globule = smoothstep(0.6, 0.8, perlin(e * 6.0 + uSeed * 4.4)) * feather;
	// Peak tau = 2.0 * depth * 0.8 (mean lumps): at the default depth (0.9) the centre transmits about 25% of the light.
	float tau = uOpticalDepth * 2.0 * density * (1.0 + globule);

	if (uPass < 0.5)
	{
		// Transmission, smooth or in 4 bands (0, 1/3, 2/3, 1) dithered on the style grid (mean over a band is exp(-tau)).
		float transmission = exp(-tau);
		if (uStylize > 0.5)
		{
			transmission = clamp(floor(transmission * 3.0 + bayer4(cell)), 0.0, 3.0) / 3.0;
		}
		gl_FragColor = vec4(vec3(transmission), 1.0);
	}
	else
	{
		// Light falls off with distance from the star, in cloud half-widths: softened inverse square.
		float dist = length(p - uIlluminator) / max(uHalfSize.x, 1.0);
		float falloff = 1.0 / (1.0 + dist * dist);
		vec3 lit = uReflectColor * uReflection * (1.0 - exp(-tau)) * falloff;

		// Warm rim: the skin of the cloud that faces the star scatters a little warm light, on its ragged, lumpy part only.
		vec2 sd = uIlluminator - uCenter;
		vec2 sdir = normalize(vec2(dot(sd, uAxis), dot(sd, perp)) / uHalfSize + vec2(1e-5));
		float facing = max(0.0, dot(normalize(e + vec2(1e-5)), sdir));
		float skin = smoothstep(0.3, 0.6, rad) * (1.0 - smoothstep(0.75, 0.95, rad));
		float ragged = smoothstep(0.45, 0.8, lumps);
		vec3 warmRim = vec3(1.0, 0.65, 0.4);
		lit += warmRim * (0.18 * uReflection * pow(facing, 2.0) * skin * ragged) * falloff;

		float peak = max(lit.r, max(lit.g, lit.b));
		lit *= min(1.0, uMaxBrightness / max(peak, 0.0001));
		if (uStylize > 0.5)
		{
			// The same 4 bands on the intensity (mean-preserving), so the scattered light steps like the extinction.
			float top = max(lit.r, max(lit.g, lit.b));
			float level = clamp(floor(top / max(uMaxBrightness, 0.0001) * 3.0 + bayer4(cell)), 0.0, 3.0) / 3.0;
			lit *= level * uMaxBrightness / max(top, 0.00001);
		}
		gl_FragColor = vec4(lit, 1.0);
	}
}
