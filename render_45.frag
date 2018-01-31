#version 450 core

uniform sampler2D texture;

void main()
{
    vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);
    if (pixel.b > 0.5) gl_FragColor = vec4(1,1,1,1);
    else gl_FragColor = pixel;
}
