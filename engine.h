#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <bitset>
#include <chrono>

#include "gui.h"
#include "player.h"
#include "scene.h"
#include "vulkan.h"

class Engine {
   public:
    Engine(uint32_t width, uint32_t height) : width(width), height(height) {}

    void run() {
        init();
        loop();
        destroy();
    }

    void init(const Vulkan::RenderPassBuilder& builder = {});
    void loop();
    void destroy();

    void add_mesh(std::unique_ptr<Shader> mesh) { scene->add_mesh(std::move(mesh)); }
    void add_gui(std::unique_ptr<Gui> gui) { guis.push_back(std::move(gui)); }
    void set_player(std::unique_ptr<Player> player) { this->player = std::move(player); }
    void set_scene(std::unique_ptr<Scene> scene) { this->scene = std::move(scene); }
    void set_bg_color(const vk::ClearColorValue& color) { vulkan.bgColor = color; }

    Player& get_player() { return *player; }
    Scene& get_scene() { return *scene; }
    bool get_key_state(size_t key) const { return key_state[key]; }
    float get_time() const { return std::chrono::duration_cast<std::chrono::duration<float>>(t - start_time).count(); }
    float get_delta_time() const { return dt.count(); }

    template <typename S>
    S& get_scene() {
        return static_cast<S&>(*scene);
    }

    template <typename P>
    P& get_player() {
        return static_cast<P&>(*player);
    }

    Vulkan vulkan;
    float mouse_dx = 0, mouse_dy = 0;

   private:
    void render();
    void handle_events(int button, int action) { player->handle_events(button, action); }

   private:
    struct EngineGui : Gui {
        EngineGui(const Engine& engine) : engine(engine), Gui("Information") {}
        virtual void gui_draw() override;
        const Engine& engine;
    };

    uint32_t width, height;
    GLFWwindow* window;

    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point t;
    std::chrono::duration<float> dt;

    float mouse_x = std::numeric_limits<float>().infinity();
    float mouse_y = std::numeric_limits<float>().infinity();

    std::bitset<GLFW_KEY_LAST> key_state;

    bool imgui_show = false;
    vk::DescriptorPool imgui_pool;

    std::unique_ptr<Scene> scene;
    std::unique_ptr<Player> player;
    std::vector<std::unique_ptr<Gui>> guis;
};
