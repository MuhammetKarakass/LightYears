#version 120

// Planet ring: a flat annulus in the planet's equatorial plane, banded and split by a Cassini-like gap. In orthographic mode DrawPlanetRing
// draws it in two halves around the planet: the front half (z >= 0) over the planet, the back half under it. In ray mode it is one pass, and
// the planet hides the ring where the ray meets the sphere first. The planet's shadow falls on the ring, and the face the star lights is
// brighter than the unlit face, which glows only where light scatters through it.
uniform int uSilhouette; // Keep actual band coverage and transparent gaps.
uniform vec2 uCenter;
uniform float uRadius;
uniform vec3 uNormal;          // unit ring normal, scene axes (x right, y down, z toward viewer)
uniform vec3 uLight;           // unit surface -> light, scene axes
uniform float uInner;          // inner edge, planet radii
uniform float uOuter;          // outer edge, planet radii
uniform float uOpacity;        // ring opacity times the planet's opacity
uniform float uSeed;           // the planet's seed: moves the band pattern
uniform float uFront;          // 1 = front half (z >= 0), 0 = back half
uniform float uBrightness;     // scales the final color (1 = ramp as authored)
uniform vec3 uLightColor;      // host star colour (display space); white = no tint
uniform float uLightIntensity; // strength of the lit face; 1 = as authored
uniform vec3 uColorA;          // dim band colour (ramp stop 2)
uniform vec3 uColorB;          // bright band colour (ramp stop 4)
uniform vec3 uRayCenter;       // ray mode: unit direction from the orbit camera to the planet centre, scene axes
uniform float uRayDistance;    // ray mode: camera distance to the centre in planet radii; 0 = orthographic
uniform vec2 uViewCenter;      // ray mode: screen point of the view centre, px
uniform vec2 uFlow;            // ray mode: unit on-screen direction the sky flows
uniform float uPixelsPerRadian; // ray mode: the sky's pixels per radian

// Band density at rr (planet radii): 0 outside inner..outer and across a gap at t = 0.62. planet.frag has the same function.
float ringDensity(float rr, float inner, float outer, float seed)
{
	float t = (rr - inner) / (outer - inner);
	float bands = 0.62 + 0.16 * sin(t * 23.0 + seed) + 0.12 * sin(t * 67.0 + seed * 2.3) + 0.08 * sin(t * 173.0 + seed * 0.7);
	float gap = smoothstep(0.01, 0.03, abs(t - 0.62));
	float edge = smoothstep(0.0, 0.05, t) * (1.0 - smoothstep(0.9, 1.0, t));
	return clamp(bands, 0.0, 1.0) * gap * edge * mix(0.65, 1.0, t);
}

// The ray through this pixel from the orbit camera (the same mapping as planet.frag).
vec3 pixelRay()
{
	vec2 o = gl_TexCoord[0].xy - uViewCenter;
	float u = dot(o, uFlow) / uPixelsPerRadian;
	float a = dot(o, vec2(-uFlow.y, uFlow.x)) / uPixelsPerRadian;
	return cos(a) * (sin(u) * vec3(uFlow, 0.0) - cos(u) * vec3(0.0, 0.0, 1.0)) + sin(a) * vec3(-uFlow.y, uFlow.x, 0.0);
}

void main()
{
	vec3 q;
	float px;
	float viewer;
	if (uRayDistance > 0.0)
	{
		vec3 dir = pixelRay();
		vec3 Pc = uRayDistance * uRayCenter;
		float denom = dot(dir, uNormal);
		if (abs(denom) < 1e-5)
		{
			discard;
		}
		// The ring plane passes through the planet's centre and the ray meets it at tr; the sphere hides that point if the ray meets it first.
		float tr = dot(Pc, uNormal) / denom;
		float b = dot(dir, Pc);
		float disc = b * b - (uRayDistance * uRayDistance - 1.0);
		float ts = b - sqrt(max(disc, 0.0));
		if (tr <= 0.0 || (disc >= 0.0 && ts > 0.0 && ts < tr))
		{
			discard;
		}
		q = tr * dir - Pc;
		px = tr / uPixelsPerRadian;
		// The camera sits at -Pc relative to the centre.
		viewer = sign(dot(-Pc, uNormal));
	}
	else
	{
		if (abs(uNormal.z) < 0.003)
		{
			discard;
		}
		// The ring plane passes through the planet's centre; the pixel's ring point sits at depth z on it.
		vec2 p = (gl_TexCoord[0].xy - uCenter) / uRadius;
		float z = -(uNormal.x * p.x + uNormal.y * p.y) / uNormal.z;
		if (uFront > 0.5 ? z < 0.0 : z >= 0.0)
		{
			discard;
		}
		q = vec3(p, z);
		px = 1.0 / uRadius;
		viewer = sign(uNormal.z);
	}
	float rr = length(q);
	float density = ringDensity(rr, uInner, uOuter, uSeed);
	if (density <= 0.001)
	{
		discard;
	}

	// Planet shadow: the ray from the ring point toward the star hits the unit sphere when b < 0 and the discriminant is positive.
	float sb = dot(q, uLight);
	float sdisc = sb * sb - (dot(q, q) - 1.0);
	float shadow = sb < 0.0 ? smoothstep(-0.03, 0.03, sdisc) : 0.0;

	// The lit face is bright; the unlit face gets only the light that scatters through the ring.
	float nl = dot(uNormal, uLight);
	float illum = sign(nl) == viewer ? 0.8 + 0.5 * abs(nl) : 0.35 + 0.35 * abs(nl);
	illum *= (1.0 - 0.92 * shadow) * uLightIntensity;
	vec3 color = mix(uColorA, uColorB, smoothstep(0.3, 0.9, density)) * mix(vec3(1.0), uLightColor, 0.7) * illum * uBrightness * 1.25;

	// Anti-aliasing: the inner and outer edges fade over one output pixel.
	float fade = smoothstep(uInner - px, uInner + px, rr) * (1.0 - smoothstep(uOuter - px, uOuter + px, rr));
	gl_FragColor = vec4(uSilhouette != 0 ? vec3(1.0) : color, clamp(density * uOpacity, 0.0, 1.0) * fade);
}
