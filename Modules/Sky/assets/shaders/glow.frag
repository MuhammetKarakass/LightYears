#version 120

// Energy glow: tiny very bright core, limited halo, nothing beyond haloRadius. Output is additive RGB.
// Glows are drawn in batches: every vertex carries its glow index in the red vertex color channel (0..MaxGlows-1),
// so one draw call covers a whole batch. Keep MaxGlows equal to VisualPipeline's GlowBatchSize.
const int MaxGlows = 32;
uniform vec4 uGlow[MaxGlows];       // xy center, z core radius, w halo radius (layer pixels)
uniform vec3 uCoreColor[MaxGlows];
uniform vec3 uHaloColor[MaxGlows];

void main()
{
	int index = int(gl_Color.r * 255.0 + 0.5);
	vec4 glow = uGlow[index];
	float d = length(gl_TexCoord[0].xy - glow.xy);
	float core = 1.0 - smoothstep(glow.z * 0.6, glow.z, d);
	float halo = pow(clamp(1.0 - d / glow.w, 0.0, 1.0), 2.4);
	gl_FragColor = vec4(uCoreColor[index] * core + uHaloColor[index] * halo * 0.6, 1.0);
}
