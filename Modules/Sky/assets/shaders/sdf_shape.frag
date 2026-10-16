#version 120

// SDF primitives: 0 circle, 1 ring, 2 capsule, 3 arc. Positions are layer pixels (texCoord == position).
uniform int uShape;
uniform vec2 uA;
uniform vec2 uB;
uniform float uRadius;
uniform float uThickness;
uniform vec2 uArc;
uniform vec4 uFill;
uniform vec4 uOutlineColor;
uniform float uOutline;
uniform float uSoft;

float coverage(float d)
{
	// uSoft == 0 snaps to the pixel grid: a pixel is in or out, decided at its center.
	return uSoft > 0.0 ? 1.0 - smoothstep(-uSoft, uSoft, d) : 1.0 - step(0.0, d);
}

float shapeDistance(vec2 p)
{
	if (uShape == 0)
	{
		return length(p - uA) - uRadius;
	}
	if (uShape == 1)
	{
		return abs(length(p - uA) - uRadius) - uThickness * 0.5;
	}
	if (uShape == 2)
	{
		vec2 pa = p - uA;
		vec2 ba = uB - uA;
		float h = clamp(dot(pa, ba) / max(dot(ba, ba), 0.0001), 0.0, 1.0);
		return length(pa - ba * h) - uRadius;
	}
	// arc: uB is the unit direction of the arc middle, uArc is (sin, cos) of the half aperture
	vec2 q = p - uA;
	vec2 local = vec2(dot(q, vec2(uB.y, -uB.x)), dot(q, uB));
	local.x = abs(local.x);
	float d = (uArc.y * local.x > uArc.x * local.y) ? length(local - uArc * uRadius) : abs(length(local) - uRadius);
	return d - uThickness * 0.5;
}

void main()
{
	float d = shapeDistance(gl_TexCoord[0].xy);
	float inner = coverage(d);
	float outer = uOutline > 0.0 ? coverage(d - uOutline) : inner;
	vec3 rgb = mix(uOutlineColor.rgb, uFill.rgb, inner);
	float alpha = outer * mix(uOutlineColor.a, uFill.a, inner);
	if (alpha <= 0.003)
	{
		discard;
	}
	gl_FragColor = vec4(rgb, alpha);
}
