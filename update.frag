#version 130
uniform sampler2D texture;
in vec2 pos;

uniform float GAME_W;
uniform float GAME_H;

void main()
{
    vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);

    int n = 0; // neigbour count
    for(int y = int(pos.y - 1); y <= pos.y + 1; ++y) {
        for (int x = int(pos.x - 1); x <= pos.x + 1; ++x) {
            if (x == int(pos.x) && y == int(pos.y)) continue;
            int tx = x; if (tx >= int(GAME_W)-1) tx = 0; if (tx < 0) tx = int(GAME_W)-1;
            int ty = y; if (ty >= int(GAME_H)-1) ty = 0; if (ty < 0) ty = int(GAME_H)-1;
            vec2 uv = vec2(float(tx)/(GAME_W-1), float(ty)/(GAME_H-1));
            vec4 neigh = texture2D(texture, uv);
            if (neigh.b > 0.5) {
                ++n;
            }
        }
    }

    const float delta_alive = 1.0/100.0;
    const float delta_dead  = 1.0/100.0;

    float c = 0;

    if (pixel.b > 0.5) {
        if (n < 2 || n > 3) {
            // cell dies
            c = 0;
            if (pixel.r < 1 - delta_dead) pixel.r += delta_dead;
        }
        else {
            // cell stays alive
            c = 1;
        }
    }
    else {
        if(n == 3) {
            // cell births
            c = 1;
            if (pixel.g < 1 - delta_alive) pixel.g += delta_alive;
        }
        else {
            // cell stays dead
            c = 0;
        }
    }
    pixel.b = c;
    gl_FragColor = pixel;
}
