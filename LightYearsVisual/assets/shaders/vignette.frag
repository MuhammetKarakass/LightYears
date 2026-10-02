#version 120

// Darkens the corners; uStrength 0 disables it. Drawn with alpha blending over the final image.
uniform vec2 uResolution;
uniform float uStrength;
uniform float uRadius;

void main()
{
	vec2 uv = gl_TexCoord[0].xy / uResolution - 0.5;
	uv.x *= uResolution.x / uResolution.y;
	float d = length(uv) / (uResolution.x / uResolution.y * 0.5 + 0.5);
	float shade = smoothstep(uRadius * 0.6, uRadius * 1.25, d);
	gl_FragColor = vec4(0.0, 0.0, 0.0, shade * uStrength);
}
