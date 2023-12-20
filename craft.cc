#include <iostream>

#include "engine.h"
#include "meshes/cloud_mesh.h"
#include "meshes/water_mesh.h"
#include "world.h"

struct McScene : Scene {
    McScene(Engine& engine) : world(std::make_unique<World>(engine)) {}

    virtual void init() override {
        world->init();
        Scene::init();
    }
    virtual void update() override {
        world->update();
        Scene::update();
    }
    virtual void draw() override {
        world->draw();
        Scene::draw();
    }

    virtual void load() override { world->load(); }
    virtual void attach() override { world->attach(); }

    std::unique_ptr<World> world;
};

struct McPlayer : Player {
    McPlayer(Engine& engine) : engine(engine), Player(engine, PLAYER_POS) {}
    virtual void handle_events(int button, int action) override {
        if (action == GLFW_PRESS) {
            if (button == GLFW_MOUSE_BUTTON_LEFT) {
                engine.get_scene<McScene>().world->voxel_handler->set_voxel();
            } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
                engine.get_scene<McScene>().world->voxel_handler->switch_mode();
            }
        }
    }

    Engine& engine;
};

int main(int argc, char* argv[]) {
    try {
        Engine engine;

        engine.set_player(std::make_unique<McPlayer>(engine));
        engine.set_scene(std::make_unique<McScene>(engine));
        engine.add_mesh(std::make_unique<CloudMesh>(engine));
        engine.add_mesh(std::make_unique<WaterMesh>(engine));

        engine.run();
    } catch (const std::exception& e) {
        std::cerr << e.what();
    } catch (...) {
        std::cerr << "Unknow exception!";
    }

    return 0;
}
