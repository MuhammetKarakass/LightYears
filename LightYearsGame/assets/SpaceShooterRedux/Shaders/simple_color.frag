#version 120

uniform sampler2D texture; // Geminin resmi
uniform vec4 customColor;  // Bizim gönderdiðimiz renk


void main()
{
    // 1. Geminin orijinal rengini al
    vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);
    
    // 2. Eðer piksel þeffafsa çizme (Boþluklarý boyama)
    if(pixel.a == 0.0)
    {
        discard;
    }
    
    // 3. Orijinal renk ile bizim rengimizi karýþtýr
    // pixel * customColor -> Gemiyi boyar ama detaylarý korur
    // Float targets do not clamp like the old 8-bit ones did; keep this content's output in [0,1].
    gl_FragColor = clamp(pixel * customColor, 0.0, 1.0);
}