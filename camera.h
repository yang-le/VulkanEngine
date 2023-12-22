#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>

struct Camera {
    Camera(float fovy, float aspect, float znear, float zfar)
        : fovy(fovy),
          fovx(2 * glm::atan(glm::tan(fovy * 0.5) * aspect)),
          proj(glm::perspective(fovy, aspect, znear, zfar)) {
        frustum.factor_y = 1.0 / std::cos(fovy * 0.5);
        frustum.tan_y = std::tan(fovy * 0.5);
        frustum.factor_x = 1.0 / std::cos(fovx * 0.5);
        frustum.tan_x = std::tan(fovx * 0.5);
    }

    virtual void update() {
        update_vectors();
        update_view_matrix();
    }

    void rotate_pitch(float delta_y) {
        constexpr float PITCH_MAX = glm::radians(89.0);

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

    float yaw = 0;
    float pitch = 0;
    float fovy, fovx;

    glm::vec3 position{0};
    glm::vec3 up{0, 1, 0};
    glm::vec3 right{1, 0, 0};
    glm::vec3 forward{0, 0, -1};
    glm::mat4 proj;
    glm::mat4 view = glm::mat4(1);

    struct Frustum {
        float factor_y;
        float tan_y;
        float factor_x;
        float tan_x;
    } frustum;
};
