#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>

#include "settings.h"

struct Camera {
    Camera(glm::vec3 position, float yaw, float pitch) : position(position), yaw(yaw), pitch(pitch) {
        up = glm::vec3(0, 1, 0);
        right = glm::vec3(1, 0, 0);
        forward = glm::vec3(0, 0, -1);

        proj = glm::perspective(V_FOV, ASPECT_RATIO, NEAR, FAR);
        view = glm::mat4(1);
    }

    virtual void update() {
        update_vectors();
        update_view_matrix();
    }

    void rotate_pitch(float delta_y) {
        pitch -= delta_y;
        pitch = glm::clamp(pitch, -PITCH_MAX, PITCH_MAX);
    }

    void rotate_yaw(float delta_x) { yaw += delta_x; }

    void move_right(float velocity) { position += right * velocity; }

    void move_left(float velocity) { position -= right * velocity; }

    void move_up(float velocity) { position += up * velocity; }

    void move_down(float velocity) { position -= up * velocity; }

    void move_forward(float velocity) { position += forward * velocity; }

    void move_back(float velocity) { position -= forward * velocity; }

    void update_view_matrix() { view = glm::lookAt(position, position + forward, up); }

    void update_vectors() {
        forward.x = glm::cos(yaw) * glm::cos(pitch);
        forward.y = glm::sin(pitch);
        forward.z = glm::sin(yaw) * glm::cos(pitch);

        forward = glm::normalize(forward);
        right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
        up = glm::normalize(glm::cross(right, forward));
    }

    float yaw;
    float pitch;
    glm::vec3 position;
    glm::vec3 up, right, forward;
    glm::mat4 proj, view;
};
