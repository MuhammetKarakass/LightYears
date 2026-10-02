#version 120

// Keeps only the part of each pixel above the threshold, hue preserved.
uniform sampler2D texture;
uniform float uThreshold;

void main()
{
	vec3 c = texture2D(texture, gl_TexCoord[0].xy).rgb;
	float peak = max(c.r, max(c.g, c.b));
	float keep = max(peak - uThreshold, 0.0) / max(peak, 0.0001);
	gl_FragColor = vec4(c * keep, 1.0);
}
