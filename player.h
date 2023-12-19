#pragma once

#include "camera.h"

struct Engine;

struct Player : Camera {
    Player(const Engine& engine, glm::vec3 position = {}, float yaw = 0, float pitch = 0)
        : engine(engine), Camera(position, yaw, pitch) {}
    virtual ~Player() = default;

    virtual void update() override {
        keyboard_control();
        mouse_control();
        Camera::update();
    }

    virtual void handle_events(int button, int action) {}

    void mouse_control();
    void keyboard_control();

    const Engine& engine;
};
