#include <iostream>
#include <SFML/Graphics.hpp>
#include <random>
#include <algorithm>
#include <vector>
#include <map>
#include <tuple>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

#ifdef _WIN32
// Choose the Nvidia card by default on optimus systems (only windows)
#include <Windows.h>
extern "C" {
	_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}
#endif

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

struct Model {
    std::string name;
    Size size;
    std::vector<unsigned char> data;

	Model(const std::string & image_file);
	Model(const fs::path & image_file);
};

struct CompiledModel {
	static std::vector<sf::Color> temp_data;
	sf::Texture tex;
	bool good = false;
};

// defined CCW
enum class Rotation {
	RIGHT,
	UP,
	LEFT,
	DOWN
};

void operator++(Rotation& r) {
	if (r == Rotation::DOWN) r = Rotation::RIGHT;
	else r = (Rotation)((int)r + 1);
}

void operator--(Rotation& r) {
	if (r == Rotation::RIGHT) r = Rotation::DOWN;
	else r = (Rotation)((int)r - 1);
}

std::vector<sf::Color> CompiledModel::temp_data;

std::vector<Model> loadModels();

namespace {
	
    const bool USE_GL45 = false; // do not use yet, doesn't work!

    sf::RenderWindow win;
#ifdef PETER_LINUX
    const std::string GAME_NAME = "floating";
#else
	const std::string GAME_NAME = "GAMU OF LIFU";
#endif
    const int GAME_W = 1024, GAME_H = 1024;
    sf::Sprite sprite;
    sf::RenderTexture *tex1 = nullptr, *tex2 = nullptr;
    sf::Texture *init_tex = nullptr;

	const float MENU_W = 250.0;

	const int PREVIEW_SCALE = 5;
    const int PREVIEW_MAX_STEP = 20;
    const int PREVIEW_W = 16, PREVIEW_H = 16;
    sf::RenderTexture *preview_tex1, *preview_tex2;
    sf::Sprite preview_sprite;
    bool do_preview = false;
    int preview_step = 0;
    sf::View preview_view;

	Rotation spawn_rotation = Rotation::RIGHT; // default rotation = 0°

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


    // max (MAX_MODEL_SIZE) ones (1) in the model definition
    // (or change the size of the "model[32]" array in the spawn.frag shader)
	std::vector<Model> models = loadModels();

    //std::vector<std::vector<sf::Vector2f>> compiled_models;
	std::vector<CompiledModel> compiled_models;

	const std::string ROTATION_STR[] = { "right", "up", "left", "down" };

	char buffer[256] = { 0 };
}

void init_models() {
    for(const Model& entry: models) {
        printf("compiling model '%s' : ", entry.name.c_str());

		compiled_models.push_back({});

        if (entry.data.size() != entry.size.x*entry.size.y) {
            printf("ERROR (skipping model): Wrong model size: %Lu != %dx%d\n", entry.data.size(), entry.size.x, entry.size.y);
            
            continue;
        }


		// create black borders for the texture
        
		CompiledModel::temp_data.reserve(entry.data.size());
		CompiledModel::temp_data.clear();
		CompiledModel &compiled_model = compiled_models.back();

		
		
		
		for (int y = 0; y < entry.size.y; y++) {
			for (int x = 0; x < entry.size.x; x++) {
				const int n = x + y * entry.size.x;
				CompiledModel::temp_data.push_back(sf::Color(0,0,entry.data[n]*255, 255));
			}
		}

		compiled_model.tex.create(entry.size.x, entry.size.y);
		compiled_model.tex.update(reinterpret_cast<sf::Uint8*>(CompiledModel::temp_data.data()));
		
		//compiled_model.tex.setRepeated(true);
		
		compiled_model.good = true;
        printf("OK\n");
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

    init_tex = new sf::Texture();
    init_tex->create(GAME_W, GAME_H);
    init_tex->update(data);
    delete [] data;

    sprite.setTexture(*init_tex);

    tex1->draw(sprite);
    run_update = false;
    delete init_tex;
    init_tex = nullptr;
}

void init() {
    if (USE_GL45) {
        sf::ContextSettings settings;
        settings.antialiasingLevel = 0;
        settings.majorVersion = 4;
        settings.minorVersion = 5;
        settings.attributeFlags = sf::ContextSettings::Core;
        settings.depthBits = 0;
        win.create(sf::VideoMode(800,800), GAME_NAME, sf::Style::Default, settings);
    } else {
        // (the "floating" title is used for tiling window managers to "float" the window)
        win.create(sf::VideoMode(800,800), GAME_NAME);
    }

    init_tex = new sf::Texture();
    init_tex->create(GAME_W, GAME_H);

    tex1 = new sf::RenderTexture();
    tex2 = new sf::RenderTexture();

    tex1->create(GAME_W, GAME_H);
    tex2->create(GAME_W, GAME_H);

	update_view.reset({ 0.0,(float)GAME_H,(float)GAME_W, (float)(-GAME_H) });

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
        update_shader.loadFromFile("shaders/update.vert", "shaders/update.frag");
        render_shader.loadFromFile("shaders/update.vert", "shaders/render.frag");
        spawn_shader.loadFromFile("shaders/update.vert", "shaders/spawn.frag");
    }

    if (!font.loadFromFile("fonts/Inconsolata-Regular.ttf")) {
        std::cout << "Failed to load font!" << std::endl;
    }
    text.setFont(font);
    text.setCharacterSize(14);


	text_bg.setSize({ MENU_W, (float)(text.getCharacterSize()*(DRAW__COUNT + models.size() + 3)) });
    text_bg.setPosition(0,0);
    text_bg.setFillColor(sf::Color(128,128,128, 240));

    default_view = win.getDefaultView();

    preview_tex1 = new sf::RenderTexture();
    preview_tex2 = new sf::RenderTexture();

    preview_tex1->create(PREVIEW_W, PREVIEW_H);
    preview_tex2->create(PREVIEW_W, PREVIEW_H);

    delete init_tex;
    init_tex = nullptr;

    init_models();

}

void start_preview() {
    sf::Uint8* data = new sf::Uint8[PREVIEW_W*PREVIEW_H*4];
    for(int i = 0; i < PREVIEW_W*PREVIEW_H; i++) {
        data[i*4 + 0] = 0;
        data[i*4 + 1] = 0;
        data[i*4 + 2] = 255*(dis(gen)>128);
        data[i*4 + 3] = 255;
    }

    init_tex = new sf::Texture();
    init_tex->create(PREVIEW_W, PREVIEW_H);
    init_tex->update(data);


	preview_view.reset({ 0.0,(float)PREVIEW_H,(float)PREVIEW_W, (float)(-PREVIEW_H) });

    preview_sprite.setTexture(*init_tex);
    preview_sprite.setPosition(0,0);
    preview_sprite.setScale(1, 1);

    preview_tex1->setView(preview_view);
    preview_tex2->setView(preview_view);

    preview_tex1->draw(preview_sprite);

   /* const auto& model = compiled_models[current_model];
    spawn_shader.setUniformArray("model", model.data(), model.size());
    spawn_shader.setUniform("model_size", (int)model.size());

    spawn_shader.setUniform("type", (int)DRAW_MODEL);
    sf::Vector2f spawn_preview = {PREVIEW_W/2, PREVIEW_H/2};
    spawn_shader.setUniform("spawn", spawn_preview);
    spawn_shader.setUniform("T", sfclock.getElapsedTime().asSeconds());

    preview_sprite.setTexture(preview_tex1->getTexture());
    preview_tex2->draw(preview_sprite, &spawn_shader);
    std::swap(preview_tex1, preview_tex2);*/
	
    delete [] data;
}

void render_preview() {
    if (do_preview) {


        //preview_step = (preview_step + 1) % PREVIEW_MAX_STEP;
        preview_step++;
        if(preview_step % (PREVIEW_MAX_STEP*10) == 0) {
            start_preview();
        }
        //if(preview_step % 10 == 0) {
        //    //start_preview();
        //    update_shader.setUniform("GAME_W", (float)PREVIEW_W);
        //    update_shader.setUniform("GAME_H", (float)PREVIEW_H);

        //    preview_sprite.setPosition(0,0);
        //    preview_sprite.setScale(1,1);
        //    preview_sprite.setTexture(preview_tex1->getTexture());
        //    preview_tex2->draw(preview_sprite, &update_shader);
        //    std::swap(preview_tex1, preview_tex2);
        //}
        preview_sprite.setTexture(preview_tex1->getTexture());
        preview_sprite.setPosition(10,200);
        preview_sprite.setScale(PREVIEW_SCALE, PREVIEW_SCALE);
        win.setView(default_view);
        win.draw(preview_sprite, &render_shader);
    }
}

void run() {
    sf::Event ev;
    start_preview();
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
                case sf::Keyboard::Escape:
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

				case sf::Keyboard::Q: //rotate CCW
					++spawn_rotation;
					break;
				case sf::Keyboard::E: // rotate CW
					--spawn_rotation;
					break;
                default:
                    break;
                }
                break;
            case sf::Event::MouseWheelScrolled:
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)) {
                    if (ev.mouseWheel.x < 0) {
                        current_model = (current_model + 1) % models.size();
                        start_preview();
                    }
                    else {
                        current_model = (current_model - 1) % models.size();
                        start_preview();
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
				default_view.reset({ 0.0,0.0,(float)ev.size.width, (float)ev.size.height });
                break;
            default:
                break;
            }
        }

        if (do_spawn) {

            if (draw_type == DRAW_MODEL) {
                /*const auto& model = compiled_models[current_model];
                spawn_shader.setUniformArray("model", model.data(), model.size());
                spawn_shader.setUniform("model_size", (int)model.size());
                do_spawn = false;*/

				const CompiledModel& model = compiled_models[current_model];
				spawn_shader.setUniform("spawn_texture", model.tex);
				spawn_shader.setUniform("spawn_rotation", (int)spawn_rotation);
				sf::Vector2f model_size = sf::Vector2f(model.tex.getSize());
				spawn_shader.setUniform("model_size", model_size);
				printf("model size = %dx%d\n", (int)model_size.x, (int)model_size.y);
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
        win.clear(sf::Color(20,20,20,255));
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

        text.setFillColor(sf::Color::White);
        for(int i = 0; i < models.size(); i++) {
            if(current_model == i) {
                text.setOutlineColor(sf::Color::Green);
                text.setOutlineThickness(1);
            }
            else {
                text.setOutlineThickness(0);
            }
            if(!compiled_models[i].good) text.setFillColor(sf::Color::Red);
            text.move(0,text.getCharacterSize());
            char buffer[32];
            sprintf(buffer, "  %s", models[i].name.c_str());
            text.setString(buffer);
            win.draw(text);
        }

        text.setOutlineThickness(0);


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
		text.move(0, text.getCharacterSize());
		
		sprintf(buffer, "rotation : %s", ROTATION_STR[(int)spawn_rotation].c_str());
		text.setString(buffer);
		win.draw(text);

        text.setFillColor(sf::Color::White);
        text.move(0,text.getCharacterSize());
        text.setString("R   : reset");
        win.draw(text);

        text.setFillColor(sf::Color::White);
        text.move(0,text.getCharacterSize());
        text.setString("N   : step");
        win.draw(text);

        render_preview();

        win.display();
    }
}

int main() {
    init();
    run();
    return 0;
}

Model::Model(const std::string & image_file) {
	Model(fs::path(image_file));
}

Model::Model(const fs::path & image_file){
	if (fs::exists(image_file)) {
		// loading image
		sf::Image img;
		img.loadFromFile(image_file.string());

		// name
		name = image_file.filename().stem().string();

		// size
		size.x = img.getSize().x;
		size.y = img.getSize().y;

		//data
		size_t surface = size.x*size.y;
		data.resize(surface);
		const sf::Uint8 * pixel_ptr = img.getPixelsPtr();
		for (size_t index = 0; index < surface; ++index) {
			data[index] = pixel_ptr[index * 4] == 255 ? 0 : 1;
		}
	}
	else {
		std::cout << "Could not load model located in " << image_file << std::endl;
	}
}

std::vector<Model> loadModels() {
	std::vector<Model> models;
	fs::path model_path("models");
	for (auto & model_file : fs::directory_iterator(model_path)) {
		if (fs::is_regular_file(model_file.path()) && (model_file.path().extension() == ".png" || model_file.path().extension() == ".jpg")) {
			models.push_back(Model(model_file.path()));
		}
	}

	return models;
}
