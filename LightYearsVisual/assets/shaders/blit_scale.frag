#version 120

// Copies a texture scaled by uScale (used to add bloom levels with a strength above 1).
// Black texels are discarded: every use either adds onto the target or writes into a target cleared to opaque
// black, so skipping them leaves the same image and saves the framebuffer read and write on the mostly dark screen.
uniform sampler2D texture;
uniform float uScale;

void main()
{
	vec3 color = texture2D(texture, gl_TexCoord[0].xy).rgb * uScale;
	if (color.r + color.g + color.b <= 0.0)
	{
		discard;
	}
	gl_FragColor = vec4(color, 1.0);
}
