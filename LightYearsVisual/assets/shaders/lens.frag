#version 120

// Schwarzschild point-lens remap of the background (VisualPipeline::SetLens), in gather form. A pixel at impact
// parameter r (from the lens centre) shows the source at beta = r - thetaE^2 / r along the same ray. For r < thetaE the
// sign flips, so the sample lands on the far side: that is the inverted inner image, handled by the same lookup.
//
// No magnification weight is applied. Gravitational lensing conserves surface brightness (specific intensity is
// constant along a ray): magnification changes the apparent solid angle of an image, so it changes the total flux of
// that image, not the brightness per unit solid angle. With backward ray tracing each screen pixel already samples the
// source at its lens-equation position, so the sampled colour is the pixel colour. Weighting by |mu| = 1/|1 - (thetaE/r)^4|
// would instead fade everything well inside the Einstein ring toward black (|mu| ~ (r/thetaE)^4 for r << thetaE), which is
// the unphysical black disc this shader used to produce.
//
// Only the shadow is dark: nothing escapes from inside it (about 2.6 Schwarzschild radii). The thin warm photon ring just
// outside it is added on top. The result is capped to the background tier (uCap, peak channel).

uniform sampler2D texture;
uniform vec2 uSize;        // output pixels
uniform vec2 uCenter;      // output pixels
uniform float uEinstein;   // output pixels, thetaE
uniform float uShadow;     // output pixels
uniform float uRingBrightness;
uniform float uCap;

void main()
{
	vec2 uv = gl_TexCoord[0].xy;
	vec2 p = uv * uSize;
	vec2 offset = p - uCenter;
	float r = max(length(offset), 1e-4);
	vec2 dir = offset / r;

	// Inside the shadow nothing escapes (about 2.6 Schwarzschild radii); soft over 2 px. This is the only dark region.
	float outside = smoothstep(uShadow - 1.0, uShadow + 1.0, r);

	// Lens equation: thetaE^2 / r = thetaE * (thetaE / r). Surface brightness is conserved, so the sample is taken
	// unweighted.
	float x = uEinstein / r;
	float beta = r - uEinstein * x;

	// Between the shadow and the Einstein ring the source lies outside the frame. Clamping the sample to the texture
	// shows the stretched edge of the sky there instead of black; only the shadow (below) is black.
	vec2 rawUv = (uCenter + dir * beta) / uSize;
	vec2 texel = vec2(0.5) / uSize;
	vec2 srcUv = clamp(rawUv, texel, vec2(1.0) - texel);
	vec3 col = texture2D(texture, srcUv).rgb;
	// Near the shadow the source runs far off the frame; the clamped edge then smeared into concentric streaks. Only
	// there (inside the ring, source outside the frame) the image fades into the sky just outside the ring in the same
	// (or, for the inner image, the opposite) direction, which reads as a smooth glow instead of lines.
	vec2 outsideBy = max(max(-rawUv, rawUv - vec2(1.0)), vec2(0.0));
	float offFrame = smoothstep(0.0, 0.2, length(outsideBy)) * (1.0 - smoothstep(uEinstein, 1.3 * uEinstein, r));
	vec2 nearUv = clamp((uCenter + dir * sign(beta) * uEinstein * 1.6) / uSize, texel, vec2(1.0) - texel);
	col = mix(col, texture2D(texture, nearUv).rgb * 0.85, offFrame) * outside;

	// Photon ring just outside the shadow, warm white, a fraction of the background cap.
	float ring = exp(-pow((r - 1.05 * uShadow) / max(0.04 * uShadow, 1.0), 2.0));
	col += vec3(1.0, 0.93, 0.80) * uRingBrightness * uCap * ring;

	float peak = max(col.r, max(col.g, col.b));
	if (peak > uCap)
	{
		col *= uCap / peak;
	}
	gl_FragColor = vec4(col, 1.0);
}
