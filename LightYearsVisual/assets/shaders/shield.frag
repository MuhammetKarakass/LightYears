#version 120

// Ship shield bubble: a faint ellipse shell (ship-local axes) with a thin bright rim. A hit brightens the rim around
// uHitAngle and sends one ripple inward. Output is additive RGB; the caller caps the brightness by luminance tier.
uniform vec2 uCenter;
uniform vec2 uAxis;
uniform vec2 uRadii;
uniform vec3 uColor;
uniform float uStrength;
uniform float uHit;
uniform float uHitAngle;
uniform float uTime;

void main()
{
	vec2 p = gl_TexCoord[0].xy - uCenter;
	vec2 q = vec2(dot(p, uAxis), dot(p, vec2(-uAxis.y, uAxis.x)));
	vec2 n = q / uRadii;
	float r = length(n);
	if (r > 1.15)
	{
		discard;
	}
	float inside = 1.0 - smoothstep(0.985, 1.0, r);
	float rim = smoothstep(0.8, 0.985, r) * inside;
	float edgeDist = (r - 0.985) * 40.0;
	float edge = exp(-edgeDist * edgeDist);
	float body = 0.06 * smoothstep(0.35, 0.95, r) * inside;
	float ang = atan(n.y, n.x);
	float shimmer = 0.8 + 0.2 * sin(ang * 9.0 + uTime * 1.6);
	float d = abs(mod(ang - uHitAngle + 3.14159265, 6.2831853) - 3.14159265);
	float arcHit = uHit * exp(-d * d * 1.6) * (0.9 * edge + 0.5 * rim);
	float waveDist = (r - (1.0 - 0.35 * uHit)) * 9.0;
	float wave = uHit * exp(-waveDist * waveDist) * 0.25 * inside;
	float v = uStrength * (body + 0.22 * rim * shimmer + 0.4 * edge * shimmer) + arcHit + wave + uHit * body * 3.0;
	// Float targets do not clamp like the old 8-bit ones did; keep this content's output in [0,1].
	gl_FragColor = clamp(vec4(uColor * v, 1.0), 0.0, 1.0);
}
