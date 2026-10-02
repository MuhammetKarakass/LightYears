#version 120

// 9-tap gaussian; uDir is the step in texture coordinates (1/width,0) or (0,1/height).
uniform sampler2D texture;
uniform vec2 uDir;

void main()
{
	vec2 uv = gl_TexCoord[0].xy;
	vec3 sum = texture2D(texture, uv).rgb * 0.2270270270;
	sum += (texture2D(texture, uv + uDir).rgb + texture2D(texture, uv - uDir).rgb) * 0.1945945946;
	sum += (texture2D(texture, uv + uDir * 2.0).rgb + texture2D(texture, uv - uDir * 2.0).rgb) * 0.1216216216;
	sum += (texture2D(texture, uv + uDir * 3.0).rgb + texture2D(texture, uv - uDir * 3.0).rgb) * 0.0540540541;
	sum += (texture2D(texture, uv + uDir * 4.0).rgb + texture2D(texture, uv - uDir * 4.0).rgb) * 0.0162162162;
	gl_FragColor = vec4(sum, 1.0);
}
