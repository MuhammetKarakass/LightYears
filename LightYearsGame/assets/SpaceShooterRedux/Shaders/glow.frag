#version 120

uniform sampler2D texture;
uniform vec3 glowColor;
uniform float glowIntensity;
uniform float time;

void main()
{
	vec4 texColor = texture2D(texture, gl_TexCoord[0].st);

	if(texColor.a < 0.01){
	discard;
	}

	// sin(time * 3.0) -> -1 ile +1 arasýnda dalgalanan bir deðer üretir.
    // * 0.3           -> -0.3 ile +0.3 arasýna sýkýþtýrýr.
    // + 0.7           -> 0.4 ile 1.0 arasýna taþýr.
    // Sonuç: Iþýk hiç sönmez (0 olmaz), sadece kýsýlýp açýlýr.
	float pulse = sin(time*3.f) *0.3 + 0.7;


	vec3 glow = glowColor * glowIntensity * pulse;

	vec3 finalColor = texColor.rgb + glow;

	gl_FragColor = vec4(finalColor, texColor.a);
}