#version 120

// Energy glow: tiny very bright core, limited halo, nothing beyond haloRadius. Output is additive RGB.
uniform vec2 uCenter;
uniform float uCoreRadius;
uniform float uHaloRadius;
uniform vec3 uCoreColor;
uniform vec3 uHaloColor;

void main()
{
	float d = length(gl_TexCoord[0].xy - uCenter);
	float core = 1.0 - smoothstep(uCoreRadius * 0.6, uCoreRadius, d);
	float halo = pow(clamp(1.0 - d / uHaloRadius, 0.0, 1.0), 2.4);
	gl_FragColor = vec4(uCoreColor * core + uHaloColor * halo * 0.6, 1.0);
}
