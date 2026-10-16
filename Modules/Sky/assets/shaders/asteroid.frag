#version 120

// One asteroid of AsteroidField, in the planets' art style (planet.frag): the surface picks flat tones from a 5-stop
// ramp with hard boundaries (rock patches, craters as tone patches with a lighter rim), the host star then shades them
// smoothly as a lit volume, and a dark one-pixel outline closes the silhouette. Far rocks fade toward the sky.
// Vertex colour: r = seed, g = radius in pixels / 510, b = host-star direction in the rock's frame (0..1 of a turn),
// a = depth (0 far .. 1 near). gl_TexCoord is the rock frame, -1..1. Straight alpha, drawn BlendAlpha.
uniform vec3 uRamp[5];
uniform vec3 uHaze;
uniform float uMaxBrightness;

const mat2 kRotate = mat2(0.8, -0.6, 0.6, 0.8);

vec2 hash22(vec2 p)
{
	vec3 p3 = fract(vec3(p.xyx) * vec3(0.1031, 0.1030, 0.0973));
	p3 += dot(p3, p3.yzx + 33.33);
	return fract((p3.xx + p3.yz) * p3.zy);
}

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
	for (int i = 0; i < 4; i++)
	{
		sum += amp * gnoise(p);
		p = kRotate * p * 2.07 + vec2(17.1, 9.3);
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

void main()
{
	vec2 p = gl_TexCoord[0].xy;
	float seed = gl_Color.r * 97.0;
	float radiusPx = max(gl_Color.g * 510.0, 1.0);
	float lightAngle = gl_Color.b * 6.2831853;
	float depth = gl_Color.a;
	float pixel = 1.0 / radiusPx;

	// Lumpy silhouette: the radius varies with direction at two scales; a crisp one-pixel anti-aliased edge.
	float r = length(p);
	vec2 dir = p / max(r, 1e-4);
	float rad = 0.72 + 0.15 * gnoise(dir * 1.3 + seed) + 0.06 * gnoise(dir * 3.7 + seed * 1.7);
	float mask = 1.0 - smoothstep(rad - pixel, rad + pixel, r);
	if (mask <= 0.002)
	{
		discard;
	}

	// Sphere-like volume stretched to the silhouette, lit softly (wrapped diffuse, dark night side) like the planets.
	vec2 q = p / rad;
	float z = sqrt(max(1.0 - dot(q, q), 0.0));
	vec3 n = normalize(vec3(q, z));
	vec3 L = normalize(vec3(cos(lightAngle), sin(lightAngle), 0.35));
	float lambert = dot(n, L);
	float daylight = smoothstep(-0.12, 0.75, lambert);

	// Surface value: rock patches, then craters as dark bowls with a lighter rim, all chosen as flat tones.
	vec2 sp = q * 1.7 + vec2(seed, seed * 0.37);
	float surface = 0.5 + fbm(sp);
	vec2 base = floor(sp * 1.3);
	for (int j = -1; j <= 1; j++)
	{
		for (int i = -1; i <= 1; i++)
		{
			vec2 c = base + vec2(float(i), float(j));
			vec2 roll = hash22(c + seed * 3.1);
			if (roll.y > 0.5)
			{
				continue;
			}
			vec2 at = (c + 0.2 + 0.6 * hash22(c * 1.7 + seed)) / 1.3;
			float radius = (0.12 + 0.2 * roll.x);
			float d = length(sp - at) / radius;
			surface = mix(surface, 0.12, 1.0 - smoothstep(0.75, 0.9, d));
			surface = max(surface, 0.78 * (1.0 - smoothstep(0.0, 0.12, abs(d - 0.95))));
		}
	}
	float value = clamp(0.5 + (surface - 0.5) * 1.7, 0.0, 1.0);
	int level = int(clamp(floor(value * 4.0 + 0.5), 0.0, 4.0));
	vec3 colour = toneColor(level) * (0.05 + 1.05 * daylight);

	// Dark outline, one or two pixels inside the edge, as the planets have.
	if (rad - r < 1.6 * pixel)
	{
		colour = uRamp[0] * 0.45;
	}

	// Distance: far rocks are dimmer and fade toward the sky colour.
	colour = mix(uHaze, colour, 0.35 + 0.65 * depth) * (0.5 + 0.5 * depth);
	gl_FragColor = vec4(colour * uMaxBrightness, mask);
}
