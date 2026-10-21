#version 120

// Host star: limb-darkened disc with a soft scattered-light halo. Drawn additively, so the output is light only.
uniform int uSilhouette; // Emitter disc with a little corona, independent of display brightness.
uniform vec2 uCenter;
uniform float uRadius;         // disc radius, output pixels
uniform float uGlowRadius;     // halo extent, output pixels; the C++ side keeps it above uRadius
uniform vec3 uColor;           // display-space blackbody colour, max channel 1
uniform float uIntensity;      // 0..1
uniform float uMaxBrightness;  // background tier cap
uniform float uRayLength;      // diffraction rays and the flare streak reach this far, output pixels

void main()
{
	vec2 d = gl_TexCoord[0].xy - uCenter;
	float r = max(length(d), 0.5);
	if (uSilhouette != 0)
	{
		float coverage = 1.0 - smoothstep(uRadius, uRadius * 1.5 + 2.0, r);
		gl_FragColor = vec4(vec3(coverage), 0.0);
		return;
	}
	if (r > uRayLength)
	{
		discard;
	}

	// Disc: I = 1 - u (1 - mu), u = 0.6, mu = cos of the angle from the disc centre.
	float rr = min(r / uRadius, 1.0);
	float mu = sqrt(1.0 - rr * rr);
	float disc = 1.0 - 0.6 * (1.0 - mu);

	// Halo: 0.35 (R / r)^2 falling smoothly to zero at the glow radius, plus a broad soft corona so the star reads as a
	// sun lighting its system instead of a grey disc.
	float fade = 1.0 - smoothstep(uRadius, uGlowRadius, r);
	float halo = 0.35 * (uRadius * uRadius) / (r * r) * fade;
	float corona = 0.4 * exp(-r / (0.15 * uGlowRadius)) + 0.12 * exp(-r / (0.45 * uGlowRadius));
	halo += corona * fade;

	// One pixel of anti-aliasing at the disc edge.
	float inside = 1.0 - smoothstep(uRadius - 1.0, uRadius + 1.0, r);
	// The limb never falls below the inner halo, or the disc gets a dark rim.
	float intensity = mix(halo, max(disc, halo), inside);

	// Short, faint diffraction rays keep the star behind combat lights.
	float a = atan(d.y, d.x);
	float rays = pow(abs(cos(a * 3.0 + 0.4)), 220.0) + 0.5 * pow(abs(cos(a * 3.0 + 1.447)), 400.0);
	float rayFade = exp(-r / (0.12 * uRayLength)) * (1.0 - smoothstep(0.3 * uRayLength, 0.6 * uRayLength, r));
	float streak = exp(-abs(d.y) / (0.012 * uRayLength)) * exp(-abs(d.x) / (0.45 * uRayLength));
	intensity += (rays * 0.12 + streak * 0.08) * rayFade * (1.0 - inside);

	// The disc burns white-hot in the middle, the star's colour shows in the halo and the rays.
	// Under the background cap a white disc would only reach grey, so the disc keeps most of the star's hue.
	// Blackbody hues are pale (a K star is beige), and pale light under the cap turns muddy, so the hue is pushed.
	float starLuma = dot(uColor, vec3(0.2126, 0.7152, 0.0722));
	vec3 vivid = max(mix(vec3(starLuma), uColor, 1.8), vec3(0.0));
	vivid /= max(max(vivid.r, max(vivid.g, vivid.b)), 1e-4);
	vec3 tint = mix(vivid, vec3(1.0), 0.3 * inside * disc);
	vec3 color = tint * intensity * uIntensity * uMaxBrightness;
	float peak = max(color.r, max(color.g, color.b));
	if (peak > uMaxBrightness)
	{
		color *= uMaxBrightness / peak;
	}
	// Alpha is not used: the draw blends colour One/One and leaves destination alpha alone (see DrawHostStar).
	gl_FragColor = vec4(color, 0.0);
}
