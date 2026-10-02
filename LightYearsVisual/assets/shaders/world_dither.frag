#version 120

// Ordered dither of the world layer: each pixel snaps to the nearest color of the active script palette
// after a Bayer offset, so off-palette colors become stippled mixes of ramp colors.
uniform sampler2D texture;
uniform vec2 uTexSize;
uniform vec3 uPalette[48];
uniform int uPaletteCount;
uniform float uSpread;

float bayer4(vec2 p)
{
	vec2 q = mod(floor(p), 4.0);
	vec2 lo = mod(q, 2.0);
	vec2 hi = floor(q / 2.0);
	float a = mod(2.0 * lo.x + 3.0 * lo.y, 4.0);
	float b = mod(2.0 * hi.x + 3.0 * hi.y, 4.0);
	return (4.0 * a + b + 0.5) / 16.0;
}

void main()
{
	vec4 c = texture2D(texture, gl_TexCoord[0].xy);
	if (c.a < 0.5)
	{
		gl_FragColor = vec4(0.0);
		return;
	}
	vec2 pixel = floor(gl_TexCoord[0].xy * uTexSize);
	vec3 target = c.rgb + vec3((bayer4(pixel) - 0.5) * uSpread);
	vec3 best = c.rgb;
	float bestDistance = 1000.0;
	for (int i = 0; i < 48; i++)
	{
		if (i >= uPaletteCount)
		{
			break;
		}
		vec3 diff = uPalette[i] - target;
		float dist = dot(diff, diff);
		if (dist < bestDistance)
		{
			bestDistance = dist;
			best = uPalette[i];
		}
	}
	gl_FragColor = vec4(best, 1.0);
}
