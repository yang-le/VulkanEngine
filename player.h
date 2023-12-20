#pragma once

#include "camera.h"

class Engine;
class Player : public Camera {
   public:
    Player(const Engine& engine, glm::vec3 position = {}, float yaw = 0, float pitch = 0)
        : engine(engine), Camera(position, yaw, pitch) {}
    virtual ~Player() = default;

    virtual void update() override {
        keyboard_control();
        mouse_control();
        Camera::update();
    }

    virtual void handle_events(int button, int action) {}

   private:
    void mouse_control();
    void keyboard_control();

    const Engine& engine;
};
