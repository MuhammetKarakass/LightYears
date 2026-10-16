#version 120

// Stylized nebula composite. The nebula target is drawn on the style-pixel grid (the ships' pixel size): each style pixel
// samples the nebula at its centre, its alpha is cut into hard tone bands with Bayer dither, and its colour snaps to the
// background palette after the same Bayer offset as world_dither.frag. The output stays premultiplied.
// Un-premultiplied alpha below this is haze, not cloud: it is dropped before banding so the dark sky carries no dither.
const float kHazeCutoff = 0.06;

uniform sampler2D texture;
uniform vec2 uSize;        // output size, output pixels
uniform float uPixel;      // style pixel size, output pixels (>= 1)
uniform float uLevels;     // alpha tone bands, 2..8
uniform float uSpread;     // colour dither offset
uniform vec3 uPalette[64];
uniform int uPaletteCount;

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
	// Sprite texture coordinates are normalized over the whole output, so the layer pixel is uv * uSize.
	vec2 cell = floor(gl_TexCoord[0].xy * uSize / uPixel);
	vec2 centre = (cell + 0.5) * uPixel;
	vec4 c = texture2D(texture, centre / uSize);
	if (c.a < kHazeCutoff)
	{
		gl_FragColor = vec4(0.0);
		return;
	}

	// Un-premultiply, then cut the alpha into bands; the threshold is the Bayer value of the style pixel.
	vec3 colour = c.rgb / max(c.a, 0.0001);
	float threshold = bayer4(cell);
	float levels = uLevels - 1.0;
	float band = clamp(floor(c.a * levels + threshold), 0.0, levels);
	float alpha = band / levels;
	if (alpha <= 0.0)
	{
		gl_FragColor = vec4(0.0);
		return;
	}

	// Snap to the nearest palette colour after the Bayer offset, as world_dither.frag does.
	vec3 target = colour + vec3((threshold - 0.5) * uSpread);
	vec3 best = colour;
	float bestDistance = 1000.0;
	for (int i = 0; i < 64; i++)
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
	gl_FragColor = vec4(best * alpha, alpha);
}
