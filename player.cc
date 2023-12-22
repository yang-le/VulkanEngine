#include "player.h"

#include "engine.h"

void Player::keyboard_control() {
    auto ds = speed * engine.get_delta_time();
    if (engine.get_key_state(GLFW_KEY_W)) move_forward(ds);
    if (engine.get_key_state(GLFW_KEY_S)) move_back(ds);
    if (engine.get_key_state(GLFW_KEY_D)) move_right(ds);
    if (engine.get_key_state(GLFW_KEY_A)) move_left(ds);
    if (engine.get_key_state(GLFW_KEY_Q)) move_up(ds);
    if (engine.get_key_state(GLFW_KEY_E)) move_down(ds);
}

void Player::mouse_control() {
    if (engine.mouse_dx) rotate_yaw(engine.mouse_dx * mouse_sensitivity);
    if (engine.mouse_dy) rotate_pitch(engine.mouse_dy * mouse_sensitivity);
}
