#version 120

// Copies a texture scaled by uScale (used to add bloom levels with a strength above 1).
uniform sampler2D texture;
uniform float uScale;

void main()
{
	gl_FragColor = vec4(texture2D(texture, gl_TexCoord[0].xy).rgb * uScale, 1.0);
}
