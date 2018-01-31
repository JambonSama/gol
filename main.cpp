#include <iostream>
#include <SFML/Graphics.hpp>
#include <random>
#include <algorithm>
#include <vector>
#include <map>
#include <tuple>
#include <string>

//using namespace std;

enum DRAW_TYPE {
    DRAW_POINT = 1,
    DRAW_RANDOM = 2,
    DRAW_CLEAR = 3,
    DRAW_MODEL = 4,
    DRAW__COUNT
};

struct Size {
    int x,y;
};

struct ModelInfo {
    std::string name;
    Size size;
    std::vector<char> data;
};

namespace {
    const bool USE_GL45 = false; // do not use yet, doesn't work!

    sf::RenderWindow win;
    const int GAME_W = 1000, GAME_H = 1000;
    sf::Sprite sprite;
    sf::RenderTexture *tex1, *tex2;
    sf::Texture init_tex;

    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<int> dis(0, 255);

    sf::Shader update_shader;
    sf::Shader render_shader;
    sf::Shader spawn_shader;

    sf::View update_view;
    sf::View render_view;
    sf::View default_view;

    sf::Vector2f drag_start;

    sf::Vector2i mouse_prev_pos;

    bool do_spawn = false;
    sf::Vector2f spawn_pos;
    DRAW_TYPE draw_type = DRAW_RANDOM;

    bool left_down = false;

    bool do_update = false;
    bool run_update = false;

    int current_model = 0;

    sf::Clock sfclock;

    sf::Font font;
    sf::Text text;
    sf::RectangleShape text_bg;

    // max 32 ones (1) in the model definition (or change the size of the "model[32]" array in the spawn.frag shader)
    const std::vector<ModelInfo> models = {
        // name, size, data
        {"glider1", {4,3},{
        0,0,1,0,
        0,0,0,1,
        0,1,1,1
        }},
        {"glider2", {4,3},{
        0,1,1,1,
        0,0,0,1,
        0,0,1,0
        }}
    };

    std::vector<std::vector<sf::Vector2f>> compiled_models;
}

void init_models() {
    for(const ModelInfo& entry: models) {
        printf("compiling model '%s' : ", entry.name.c_str());
        int x = 0, y = 0;
        std::vector<sf::Vector2f> compiled_model;
        for(int n = 0; n < entry.data.size(); n++) {
            if (entry.data[n] == 1) {
                compiled_model.push_back({x,y});
            }

            if (x == entry.size.x - 1) {
                x = 0;
                y++;
            } else {
                x++;
            }
        }
        compiled_models.push_back(compiled_model);
        printf("%d points\n", compiled_model.size());
    }
}

void init_world() {
    sf::Uint8* data = new sf::Uint8[GAME_W*GAME_H*4];
    for(int i = 0; i < GAME_W*GAME_H; i++) {
        //const sf::Uint8 c = 255*(dis(gen) > 128);
        data[i*4 + 0] = 0;
        data[i*4 + 1] = 0;
        data[i*4 + 2] = 0;
        data[i*4 + 3] = 255;
    }

    init_tex.update(data);
    delete [] data;

    sprite.setTexture(init_tex);

    tex1->draw(sprite);

    run_update = false;
}

void init() {
    if (USE_GL45) {
        sf::ContextSettings settings;
        settings.antialiasingLevel = 0;
        settings.majorVersion = 4;
        settings.minorVersion = 5;
        settings.attributeFlags = sf::ContextSettings::Core;
        settings.depthBits = 0;
        win.create(sf::VideoMode(800,800),"floating", sf::Style::Default, settings);
    } else {
        // (the "floating" title is used for tiling window managers to "float" the window)
        win.create(sf::VideoMode(800,800), "floating");
    }
    init_tex.create(GAME_W, GAME_H);

    tex1 = new sf::RenderTexture();
    tex2 = new sf::RenderTexture();

    tex1->create(GAME_W, GAME_H);
    tex2->create(GAME_W, GAME_H);

    update_view.reset({0,GAME_H,GAME_W, -GAME_H});

    tex1->setView(update_view);
    tex2->setView(update_view);

    tex1->setSmooth(false);
    tex2->setSmooth(false);

    init_world();

    render_view = update_view;
    win.setView(render_view);

    win.setVerticalSyncEnabled(false);
    win.setFramerateLimit(60);

    if (USE_GL45) {
        //update_shader.loadFromFile("update_45.vert", "update_45.frag");
        //render_shader.loadFromFile("update_45.vert", "render_45.frag");
    }
    else {
        update_shader.loadFromFile("update.vert", "update.frag");
        render_shader.loadFromFile("update.vert", "render.frag");
        spawn_shader.loadFromFile("update.vert", "spawn.frag");
    }


    font.loadFromFile("fonts/Inconsolata-Regular.ttf");
    text.setFont(font);
    text.setCharacterSize(14);


    text_bg.setSize({100, text.getCharacterSize()*(DRAW__COUNT + models.size() + 3)});
    text_bg.setPosition(0,0);
    text_bg.setFillColor(sf::Color(128,128,128, 240));

    default_view = win.getDefaultView();

    init_models();
}

void run() {
    sf::Event ev;
    while(win.isOpen()) {

        while(win.pollEvent(ev)) {
            switch (ev.type) {
            case sf::Event::Closed:
                win.close();
                break;
            case sf::Event::KeyPressed:
                switch(ev.key.code) {
                case sf::Keyboard::N:
                    do_update = true;
                    run_update = false;
                    break;
                case sf::Keyboard::Q:
                    win.close();
                    break;
                case sf::Keyboard::Space:
                    run_update = !run_update;
                    break;
                case sf::Keyboard::R:
                    init_world();
                    break;
                case sf::Keyboard::Num1:
                    draw_type = DRAW_POINT;
                    break;
                case sf::Keyboard::Num2:
                    draw_type = DRAW_RANDOM;
                    break;
                case sf::Keyboard::Num3:
                    draw_type = DRAW_CLEAR;
                    break;
                case sf::Keyboard::Num4:
                    draw_type = DRAW_MODEL;
                    break;
                default:
                    break;
                }
                break;
            case sf::Event::MouseWheelScrolled:
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)) {
                    if (ev.mouseWheel.x < 0) {
                        current_model = (current_model + 1) % models.size();
                    }
                    else {
                        current_model = (current_model - 1) % models.size();
                    }
                }
                else {
                    if (ev.mouseWheel.x < 0) {
                        render_view.zoom(1.1);
                    }
                    else {
                        render_view.zoom(0.9);
                    }
                    }
                break;

            case sf::Event::MouseButtonPressed:
                if (ev.mouseButton.button == sf::Mouse::Left) {
                    left_down = true;
                    mouse_prev_pos = {ev.mouseButton.x, ev.mouseButton.y};
                }
                else if(ev.mouseButton.button == sf::Mouse::Right) {
                    do_spawn = true;
                }
                break;
            case sf::Event::MouseButtonReleased:
                if (ev.mouseButton.button == sf::Mouse::Left) {
                    left_down = false;
                }
                else if(ev.mouseButton.button == sf::Mouse::Right) {
                    do_spawn = false;
                }
                break;

            case sf::Event::MouseMoved:
                {
                    sf::Vector2i mouse_pos = {ev.mouseMove.x, ev.mouseMove.y};
                    sf::Vector2f delta = win.mapPixelToCoords(mouse_pos, render_view) - win.mapPixelToCoords(mouse_prev_pos, render_view);
                    mouse_prev_pos = mouse_pos;
                    spawn_pos = win.mapPixelToCoords(mouse_pos, render_view);
                    if (left_down) {
                        render_view.move(-delta.x, -delta.y);
                    }
                }
                break;
            case sf::Event::Resized:
                render_view.setSize(ev.size.width, ev.size.height);
                default_view.reset({0,0,ev.size.width, ev.size.height});
                break;
            default:
                break;
            }
        }

        if (do_spawn) {

            if (draw_type == DRAW_MODEL) {
                const auto& model = compiled_models[current_model];
                spawn_shader.setUniformArray("model", model.data(), model.size());
                spawn_shader.setUniform("model_size", (int)model.size());
                do_spawn = false;
            }
            spawn_shader.setUniform("type", (int)draw_type);
            spawn_shader.setUniform("spawn", spawn_pos);
            spawn_shader.setUniform("T", sfclock.getElapsedTime().asSeconds());

            sprite.setTexture(tex1->getTexture());
            tex2->draw(sprite, &spawn_shader);
            std::swap(tex1, tex2);
        }

        if (run_update || do_update) {
            update_shader.setUniform("GAME_W", (float)GAME_W);
            update_shader.setUniform("GAME_H", (float)GAME_H);

            sprite.setTexture(tex1->getTexture());
            tex2->draw(sprite, &update_shader);
            std::swap(tex1, tex2);
            do_update = false;
        }

        sprite.setTexture(tex1->getTexture());
        win.setView(render_view);
        win.clear();
        win.draw(sprite, &render_shader);


        win.setView(default_view);
        win.draw(text_bg);
        if(draw_type == DRAW_POINT) text.setFillColor(sf::Color::Green); else text.setFillColor(sf::Color::White);
        text.setPosition(5,0);
        text.setString("1   : point");
        win.draw(text);

        if(draw_type == DRAW_RANDOM) text.setFillColor(sf::Color::Green); else text.setFillColor(sf::Color::White);
        text.move(0,text.getCharacterSize());
        text.setString("2   : random");
        win.draw(text);

        if(draw_type == DRAW_CLEAR) text.setFillColor(sf::Color::Green); else text.setFillColor(sf::Color::White);
        text.move(0,text.getCharacterSize());
        text.setString("3   : clear");
        win.draw(text);

        if(draw_type == DRAW_MODEL) text.setFillColor(sf::Color::Green); else text.setFillColor(sf::Color::White);
        text.move(0,text.getCharacterSize());
        text.setString("4   : model");
        win.draw(text);

        for(int i = 0; i < models.size(); i++) {
            if(current_model == i) text.setFillColor(sf::Color::Green); else text.setFillColor(sf::Color::White);
            text.move(0,text.getCharacterSize());
            char buffer[32];
            sprintf(buffer, "  %s", models[i].name.c_str());
            text.setString(buffer);
            win.draw(text);
        }


        if(run_update) {
             text.setFillColor(sf::Color::Green);
             text.setString("spc : running");
        }
        else {
            text.setFillColor(sf::Color::Red);
            text.setString("spc : stopped");
        }
        text.move(0,text.getCharacterSize());
        win.draw(text);

        text.setFillColor(sf::Color::White);
        text.move(0,text.getCharacterSize());
        text.setString("R   : reset");
        win.draw(text);

        text.setFillColor(sf::Color::White);
        text.move(0,text.getCharacterSize());
        text.setString("N   : step");
        win.draw(text);

        win.display();
    }
}

int main()
{
    init();
    run();
    return 0;
}
