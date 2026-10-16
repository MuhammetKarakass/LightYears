#version 120

// Rotating neutron star, lighthouse model. The spin axis is the screen vertical. The magnetic axis is tilted from the
// spin axis by uTilt and precesses with uPhase. Its sky projection is s = (sin(tilt) cos(phase), cos(tilt)) (x across,
// y along the spin axis) and its depth toward the viewer is z = sin(tilt) sin(phase). The opposite pole projects to -s
// with depth -z. Each beam is brightest when its pole points at us, which is the pulse.
// The wind nebula is steady (synchrotron torus and jets around the spin axis) with filaments in H-alpha at its edge.
// Everything is additive and normalised so the peak channel never exceeds uMaxBrightness (background tier).

uniform vec2 uCenter;
uniform float uPhase;
uniform float uTilt;
uniform float uBeamLength;
uniform float uBeamHalfAngle;
uniform float uNebulaRadius;
uniform float uMaxBrightness;

// Synchrotron continuum is a power law, so the nebula and the beams are blue-white.
const vec3 kSynchrotron = vec3(0.70, 0.82, 1.00);
const vec3 kCore = vec3(0.85, 0.92, 1.00);
// H-alpha 656 nm, the warm filaments of the shocked shell.
const vec3 kHalpha = vec3(1.00, 0.12, 0.10);

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
	for (int i = 0; i < 3; i++)
	{
		sum += amp * valueNoise(p);
		p = p * 2.03 + vec2(17.1, 9.2);
		amp *= 0.5;
	}
	return sum;
}

// One magnetic-pole beam: a cone of half-angle uBeamHalfAngle along dir that fades with distance.
float Beam(vec2 p, vec2 dir, float visibility, float reach)
{
	float along = dot(p, dir);
	if (along <= 0.0 || along > reach)
	{
		return 0.0;
	}
	float perp = length(p - along * dir);
	float halfWidth = along * uBeamHalfAngle + 0.6; // 0.6 px floor keeps the beam visible next to the core
	float across = exp(-2.0 * pow(perp / halfWidth, 2.0));
	float fade = exp(-along / (0.45 * reach));
	return visibility * across * fade / (1.0 + 0.15 * along);
}

void main()
{
	vec2 p = gl_TexCoord[0].xy - uCenter;
	float r = length(p);

	// Projected magnetic axis (see the header comment). sin(tilt) cancels in the depth, so depth = sin(phase).
	float sinT = sin(uTilt);
	vec2 s = vec2(sinT * cos(uPhase), cos(uTilt));
	float sLen = max(length(s), 1e-4);
	vec2 sHat = s / sLen;
	float reach = uBeamLength * sLen; // a beam shortens when the axis is seen end-on

	float zn = sin(uPhase);
	float visUp = 0.12 + 0.88 * pow(0.5 + 0.5 * zn, 3.0);
	float visDown = 0.12 + 0.88 * pow(0.5 - 0.5 * zn, 3.0);
	float beams = Beam(p, sHat, visUp, reach) + Beam(p, -sHat, visDown, reach);

	// Pulsar itself: a point source, roughly 1.5 px across.
	float core = exp(-r * r / (2.0 * 0.8 * 0.8));

	// Wind nebula: a torus in the plane perpendicular to the spin axis (squashed by the viewing angle) plus polar jets.
	float R = uNebulaRadius;
	float rr = length(vec2(p.x, p.y / 0.45));
	float torus = exp(-pow((rr - 0.55 * R) / (0.22 * R), 2.0));
	float jet = exp(-pow(p.x / (0.07 * R), 2.0)) * exp(-abs(p.y) / (0.6 * R)) * 0.6;
	float edge = 1.0 - smoothstep(0.75 * R, R, rr);
	vec3 nebula = kSynchrotron * (0.55 * torus + jet) * edge;

	// H-alpha filaments: noise on a circle (no seam, no symmetric petals) keyed to the shell radius.
	float ang = atan(p.y, p.x);
	vec2 polar = vec2(cos(ang), sin(ang)) * 1.7;
	float warp = fbm(polar + vec2(rr * 0.02, 0.0));
	float filament = smoothstep(0.52, 0.80, fbm(polar * 2.1 + vec2(warp * 2.0, rr * 0.035)));
	float rim = smoothstep(0.55 * R, 0.95 * R, rr) * (1.0 - smoothstep(R, 1.15 * R, rr));
	vec3 halpha = kHalpha * filament * rim * 0.5;

	vec3 col = nebula + halpha + kSynchrotron * beams + kCore * core;

	// Shape is kept, the peak is limited to the background tier.
	float peak = max(col.r, max(col.g, col.b));
	gl_FragColor = vec4(col * (uMaxBrightness / max(1.0, peak)), 1.0);
}
