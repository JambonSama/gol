#version 130
uniform sampler2D texture; // used by SFML for sprite rendering
uniform sampler2D spawn_texture;
uniform vec2 model_size;
uniform vec2 spawn;

in vec2 pos;

uniform float T;

uniform int type;

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
        
        // if the current pixel corresponds to an alive cell in the spawn texture,
        // turn it alive
        vec2 spawn_uv = (pos - spawn)/model_size;
        vec4 spawn_pixel = texture2D(spawn_texture, spawn_uv);
        if (spawn_pixel.b > 0.5) {
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
