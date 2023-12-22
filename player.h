#pragma once

#include "camera.h"

class Engine;
class Player : public Camera {
   public:
    Player(const Engine& engine, float speed = 1, float mouse_sensitivity = 0.002, float fovy = glm::radians(50.0),
           float aspect = 1, float znear = 0.1, float zfar = 2000)
        : engine(engine), speed(speed), mouse_sensitivity(mouse_sensitivity), Camera(fovy, aspect, znear, zfar) {}
    virtual ~Player() = default;

    virtual void update() override {
        keyboard_control();
        mouse_control();
        Camera::update();
    }

    virtual void handle_events(int button, int action) {}

    float speed;

   private:
    void mouse_control();
    void keyboard_control();

    const Engine& engine;
    float mouse_sensitivity;
};
