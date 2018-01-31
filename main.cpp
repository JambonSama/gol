#include <iostream>
#include <SFML/Graphics.hpp>
#include <random>
#include <algorithm>

//using namespace std;

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

    sf::Vector2f drag_start;

    sf::Vector2i mouse_prev_pos;

    bool do_spawn = false;
    sf::Vector2f spawn_pos;

    bool left_down = false;

    sf::Clock sfclock;
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

    sf::Uint8* data = new sf::Uint8[GAME_W*GAME_H*4];
    for(int i = 0; i < GAME_W*GAME_H; i++) {
        const sf::Uint8 c = 255*(dis(gen) > 128);
        data[i*4 + 0] = 0;
        data[i*4 + 1] = 0;
        data[i*4 + 2] = c;
        data[i*4 + 3] = 255;
    }

    init_tex.create(GAME_W, GAME_H);
    init_tex.update(data);
    delete [] data;

    tex1 = new sf::RenderTexture();
    tex2 = new sf::RenderTexture();

    tex1->create(GAME_W, GAME_H);
    tex2->create(GAME_W, GAME_H);

    update_view.reset({0,GAME_H,GAME_W, -GAME_H});

    tex1->setView(update_view);
    tex2->setView(update_view);

    tex1->setSmooth(false);
    tex2->setSmooth(false);

    sprite.setTexture(init_tex);

    tex1->draw(sprite);

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
}

void run() {
    sf::Event ev;
    while(win.isOpen()) {

        bool do_update = true;

        while(win.pollEvent(ev)) {
            switch (ev.type) {
            case sf::Event::Closed:
                win.close();
                break;
            case sf::Event::KeyPressed:
                if(ev.key.code == sf::Keyboard::N) {
                    do_update = true;
                }
                else if(ev.key.code == sf::Keyboard::Q) {
                    win.close();
                }
                break;
            case sf::Event::MouseWheelScrolled:
                if (ev.mouseWheel.x < 0) {
                    render_view.zoom(1.1);
                }
                else {
                    render_view.zoom(0.9);
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
                    sf::Vector2f delta = win.mapPixelToCoords(mouse_pos) - win.mapPixelToCoords(mouse_prev_pos);
                    mouse_prev_pos = mouse_pos;
                    spawn_pos = win.mapPixelToCoords(mouse_pos);
                    if (left_down) {
                        render_view.move(-delta.x, -delta.y);
                        win.setView(render_view);
                    }
                }
                break;
            case sf::Event::Resized:
                render_view.setSize(ev.size.width, ev.size.height);
                break;
            default:
                break;
            }
        }

        if (do_spawn) {
            spawn_shader.setUniform("spawn", spawn_pos);
            spawn_shader.setUniform("T", sfclock.getElapsedTime().asSeconds());

            sprite.setTexture(tex1->getTexture());
            tex2->draw(sprite, &spawn_shader);
            std::swap(tex1, tex2);
        }

        if (do_update) {
            update_shader.setUniform("GAME_W", (float)GAME_W);
            update_shader.setUniform("GAME_H", (float)GAME_H);

            sprite.setTexture(tex1->getTexture());
            tex2->draw(sprite, &update_shader);
            std::swap(tex1, tex2);
        }

        sprite.setTexture(tex1->getTexture());
        win.setView(render_view);
        win.clear();
        win.draw(sprite, &render_shader);

        win.display();
    }
}

int main()
{
    init();
    run();
    return 0;
}
