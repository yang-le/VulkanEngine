#pragma once

#include "camera.h"

struct Engine;

struct Player : Camera {
    Player(Engine* engine, glm::vec3 position = PLAYER_POS, float yaw = glm::radians(-90.0f), float pitch = 0)
        : engine(engine), Camera(position, yaw, pitch) {}

    virtual void update() override {
        keyboard_control();
        mouse_control();
        Camera::update();
    }

    void handle_events(int button, int action);
    void mouse_control();
    void keyboard_control();

    Engine* engine;
};
