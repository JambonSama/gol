#version 130
uniform sampler2D texture;
in vec2 pos;
uniform vec2 spawn;
uniform float T;

uniform int type;

uniform vec2 model[512];
uniform int model_size;

float rand(vec2 co){
    return fract(T*sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

void main()
{
    vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);
    // DRAW_POINT
    if (type == 1) {
        if (int(pos.x) == int(spawn.x) && int(pos.y) == int(spawn.y)) {
            gl_FragColor = vec4(pixel.rg, 1, 1);
        }
        else {
            gl_FragColor = pixel;
        }
    }
    // DRAW_RANDOM
    else if (type == 2) {
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
    // DRAW_CLEAR
    else if (type == 3) {
        if ( distance(pos, spawn) < 20 ) {
            gl_FragColor = vec4(pixel.rg, 0, 1);
        }
        else {
            gl_FragColor = pixel;
        }
    }
    else if (type == 4) {
        bool had_pixel = false;
        for(int i = 0; i < model_size; i++) {
            if (int(model[i].x + spawn.x) == int(pos.x) && int(model[i].y + spawn.y) == int(pos.y)) {
                had_pixel = true;
                break;
            }
        }
        if (had_pixel) {
            gl_FragColor = vec4(pixel.rg, 1, 1);
        }
        else {
            gl_FragColor = pixel;
        }
    }
    else {
        gl_FragColor = pixel;
    }
}
