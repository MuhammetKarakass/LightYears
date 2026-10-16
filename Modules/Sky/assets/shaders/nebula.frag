#version 120

// Soft low-frequency domain-warped clouds colored by a 4-stop ramp. Drawn with alpha blending.
uniform vec2 uScroll;
uniform float uScale;
uniform float uIntensity;
uniform float uSeed;
uniform float uMaxBrightness;
uniform vec3 uRamp[4];

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

vec3 rampAt(float t)
{
	float x = clamp(t, 0.0, 1.0) * 3.0;
	vec3 c = mix(uRamp[0], uRamp[1], clamp(x, 0.0, 1.0));
	c = mix(c, uRamp[2], clamp(x - 1.0, 0.0, 1.0));
	return mix(c, uRamp[3], clamp(x - 2.0, 0.0, 1.0));
}

void main()
{
	vec2 p = (gl_TexCoord[0].xy + uScroll) * uScale + vec2(uSeed * 37.1, uSeed * 11.7);
	vec2 warp = vec2(fbm(p + 3.1), fbm(p + 7.7));
	float density = fbm(p + 1.6 * warp);
	float cloud = smoothstep(0.38, 0.88, density);
	vec3 color = rampAt(density * 1.15);
	float peak = max(color.r, max(color.g, color.b));
	color *= min(1.0, uMaxBrightness / max(peak, 0.0001));
	gl_FragColor = vec4(color, cloud * uIntensity);
}
