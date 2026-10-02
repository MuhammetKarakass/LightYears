#version 120

// Procedural planet: SDF sphere + surface noise quantized to a ramp with hard tone boundaries,
// Bayer dither only near tone boundaries, lit/dark side, outline and rim light. Drawn in the world layer.
uniform vec2 uCenter;
uniform float uRadius;
uniform float uPixel;          // output pixels per world pixel
uniform vec3 uLight;           // surface -> light, scene axes (x right, y down, z toward viewer)
uniform float uRotation;
uniform float uSeed;
uniform float uDither;         // 0 = hard bands only, 1 = wide dither
uniform float uBrightness;     // scales the final color (1 = ramp as authored)
uniform vec3 uRamp[5];

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

float noise3(vec3 x)
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
		sum += amp * noise3(p);
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

void main()
{
	vec2 p = (gl_TexCoord[0].xy - uCenter) / uRadius;
	float r2 = dot(p, p);
	if (r2 > 1.0)
	{
		discard;
	}
	float z = sqrt(1.0 - r2);
	vec3 n = vec3(p.x, p.y, z);

	// Surface coordinates live on the sphere, so bands and continents compress toward the limb by themselves.
	float c = cos(uRotation);
	float s = sin(uRotation);
	vec3 m = vec3(n.x * c + n.z * s, n.y, -n.x * s + n.z * c);
	float continents = fbm3(m * 3.2 + uSeed);
	float bands = sin(m.y * 7.0 + continents * 3.5) * 0.5 + 0.5;
	float surface = mix(continents, bands, 0.35);

	float lambert = dot(n, normalize(uLight));
	float shade = clamp(lambert * 0.85 + 0.22, 0.0, 1.0);
	float value = shade * 0.78 + (surface - 0.5) * 0.34;

	const float tones = 5.0;
	float threshold = 0.5 + (bayer4(gl_FragCoord.xy) - 0.5) * uDither;
	float level = clamp(floor(value * (tones - 1.0) + threshold), 0.0, tones - 1.0);
	vec3 color = toneColor(int(level));

	float edge = 1.0 - sqrt(r2);
	float pixelEdge = uPixel / uRadius;
	if (edge < pixelEdge)
	{
		color = uRamp[0] * 0.45;
	}
	else if (edge < pixelEdge * 2.0 && lambert > 0.25)
	{
		color = uRamp[4];
	}
	gl_FragColor = vec4(color * uBrightness, 1.0);
}
