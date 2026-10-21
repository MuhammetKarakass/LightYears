#version 120

// Colour grade of the finished background, drawn once in VisualPipeline::Compose (after the lens, before the world).
// Works in normalized space n = c / uCap, so the grade never lifts the background above its tier cap.
// Order: black point, luma-preserving S-curve, saturation, luma-preserving split tone, then a hue-keeping limit to 1.

uniform sampler2D texture;
uniform int uClampOnly; // Shafts still respect the Background ceiling when artistic grading is disabled.
uniform float uCap;
uniform float uContrast;
uniform float uSaturation;
uniform float uBlackPoint;
uniform vec3 uShadowTint;
uniform float uShadowStrength;
uniform vec3 uHighlightTint;
uniform float uHighlightStrength;

void main()
{
	const vec3 lumaWeights = vec3(0.2126, 0.7152, 0.0722);
	vec4 c = texture2D(texture, gl_TexCoord[0].xy);
	float cap = max(uCap, 1e-4);
	if (uClampOnly != 0)
	{
		gl_FragColor = vec4(min(c.rgb, vec3(cap)), c.a);
		return;
	}

	// Per-channel black point: values under uBlackPoint (output units) crush to black, deep space stays deep.
	float bp = min(uBlackPoint / cap, 0.95);
	vec3 n = max(c.rgb / cap - bp, vec3(0.0)) / (1.0 - bp);

	// S-curve on luma, pivot at mid-grey of the cap. contrast 1 is identity; the smoothstep is monotonic.
	float l = dot(n, lumaWeights);
	float curved = clamp(mix(l, l * l * (3.0 - 2.0 * l), uContrast - 1.0), 0.0, 1.0);
	n *= curved / max(l, 1e-4);

	// Saturation around the graded luma keeps that luma.
	n = max(mix(vec3(curved), n, uSaturation), vec3(0.0));

	// Split tone: shadows lean to the shadow tint, highlights to the highlight tint. Each tint is divided by its own
	// luma, so the tint changes the hue only and the grade does not brighten or darken anything.
	float shadowWeight = (1.0 - curved) * (1.0 - curved) * uShadowStrength;
	float highlightWeight = curved * curved * uHighlightStrength;
	vec3 shadowTarget = n * (uShadowTint / max(dot(uShadowTint, lumaWeights), 1e-4));
	vec3 highlightTarget = n * (uHighlightTint / max(dot(uHighlightTint, lumaWeights), 1e-4));
	n = mix(n, shadowTarget, shadowWeight);
	n = mix(n, highlightTarget, highlightWeight);

	// Scale down, not clip, so a bright hue keeps its colour. The output never exceeds the tier cap.
	n = max(n, vec3(0.0));
	n /= max(max(n.r, max(n.g, n.b)), 1.0);
	gl_FragColor = vec4(n * cap, c.a);
}
