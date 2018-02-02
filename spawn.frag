#version 130
uniform sampler2D texture; // used by SFML for sprite rendering
uniform sampler2D spawn_texture;
uniform vec2 model_size;
uniform vec2 spawn;

in vec2 pos;

uniform float T;

uniform int type;
uniform int spawn_rotation; //0 = RIGHT; 1 = UP; 2 = LEFT, 3 = DOWN

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
		vec2 _model_size = model_size;
		if (spawn_rotation == 1 || spawn_rotation == 3) {
			_model_size.xy = model_size.yx;
		}
        vec2 _spawn_uv = vec2( (pos.x + _model_size.x/2.0 - spawn.x)/_model_size.x, (pos.y + _model_size.y/2.0 - spawn.y)/_model_size.y);
		vec2 spawn_uv = _spawn_uv;
		if (spawn_rotation == 0) {
			// default value
		}
		else if (spawn_rotation == 1) {
			// UP
			spawn_uv.x = _spawn_uv.y;
			spawn_uv.y = 1 - _spawn_uv.x;
		}
		else if (spawn_rotation == 2) {
			// LEFT
			spawn_uv.x = 1 - _spawn_uv.x;
			spawn_uv.y = 1 - _spawn_uv.y;
		}
		else if (spawn_rotation == 3) {
			// DOWN
			spawn_uv.x =  1 -_spawn_uv.y;
			spawn_uv.y =  _spawn_uv.x;
		}
        vec4 spawn_pixel = texture2D(spawn_texture, spawn_uv);
        if (spawn_pixel.b > 0.5 && spawn_uv.x >= 0 && spawn_uv.y >= 0 && spawn_uv.x <= 1 && spawn_uv.y <= 1) {
            gl_FragColor = vec4(pixel.rg, 1, 1);
			//gl_FragColor = vec4(spawn_uv, 0, 1);
        }
        else {
            gl_FragColor = pixel;   
        }
    }
    else {
        gl_FragColor = pixel;
    }
}
