#version 120

// Comet: a bright nucleus in a soft coma and two tails that point away from the host star. The ion tail is straight and
// blue (ions follow the solar wind and glow by fluorescence); the dust tail is broader, curves back along the orbit and
// shines by reflected sunlight. Drawn additively, so the output is light only.
uniform vec2 uHead;            // nucleus, output pixels
uniform vec2 uAway;            // unit vector pointing away from the star
uniform float uLength;         // tail length, output pixels
uniform float uWidth;          // coma radius, output pixels
uniform float uBend;           // dust tail curvature, signed: which side it trails
uniform vec3 uDustColor;
uniform vec3 uIonColor;
uniform float uTime;
uniform float uMaxBrightness;

void main()
{
	vec2 rel = gl_TexCoord[0].xy - uHead;
	vec2 perp = vec2(-uAway.y, uAway.x);
	float s = dot(rel, uAway);
	float t = dot(rel, perp);
	float along = clamp(s / uLength, 0.0, 1.0);
	float behind = step(0.0, s) * (1.0 - smoothstep(0.85, 1.0, s / uLength));

	// Ion tail: thin and straight, a little wider far out, with faint streamers that drift outward.
	float ionWidth = uWidth * (0.18 + 0.5 * along);
	float ion = exp(-(t * t) / (ionWidth * ionWidth)) * pow(1.0 - along, 1.4) * behind;
	ion *= 0.8 + 0.2 * sin(s * 0.05 - uTime * 1.5 + t * 0.3);

	// Dust tail: curves back along the orbit and fans out, with soft striations.
	float t2 = t - uBend * along * along * uLength * 0.22;
	float dustWidth = uWidth * (0.35 + 1.6 * along);
	float dust = exp(-(t2 * t2) / (dustWidth * dustWidth)) * pow(1.0 - along, 1.1) * behind;
	dust *= 0.75 + 0.25 * sin(t2 / max(dustWidth, 1.0) * 7.0 + s * 0.01);

	// Coma and nucleus.
	float r = length(rel);
	float coma = exp(-r / (uWidth * 0.45));
	float nucleus = exp(-r * r / (uWidth * uWidth * 0.02));

	vec3 c = uDustColor * (dust * 1.0 + coma * 0.7) + uIonColor * ion * 0.9 + vec3(1.0) * nucleus * 0.9;
	c *= uMaxBrightness;
	float peak = max(c.r, max(c.g, c.b));
	if (peak > uMaxBrightness)
	{
		c *= uMaxBrightness / peak;
	}
	gl_FragColor = vec4(c, 0.0);
}
