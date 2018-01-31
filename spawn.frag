#version 130
uniform sampler2D texture;
in vec2 pos;
uniform vec2 spawn;
uniform float T;

float rand(vec2 co){
    return fract(T*sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

void main()
{
    vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);

    if ( distance(pos, spawn) < 20 ) {
        if (rand(pos) > 0.5) {
            gl_FragColor = vec4(pixel.rg, 1, 1);
        } else {
            gl_FragColor = vec4(pixel.rg, 0, 1);
        }
    }
    else {
        gl_FragColor = pixel;
    }
}
