#version 120

uniform vec3 lightColor;
uniform float lightIntensity;
uniform float u_stretch;
uniform float u_is_trail;
uniform float u_taper;
uniform float u_edge_softness;
uniform float u_roundness;

void main()
{
	vec2 center= vec2(0.5, 0.5);

    if(u_is_trail > 0.5)
    {
        center.y= 0.05;
    }

	vec2 pos = gl_TexCoord[0].xy - center;

     if (u_is_trail > 0.5 && pos.y < 0.0)
    {
        gl_FragColor = vec4(0.0);
        return;
    }

    if (u_is_trail > 0.5)
    {
        if (pos.y < 0.0)
        {
            pos.y *= u_stretch;
        }
        else
        {

            pos.x *= (1.0 + pos.y * u_taper); 
        }
    }

	float distCircle = length(pos);
    float distBox = max(abs(pos.x), abs(pos.y));
    float distance = mix(distCircle, distBox, u_roundness);


    float innerRadius = 0.5 *(1.0 - u_edge_softness);
	float alpha = 1.0 - smoothstep(innerRadius, 0.5, distance);

	alpha *= lightIntensity;
	gl_FragColor = vec4(lightColor, alpha);
}