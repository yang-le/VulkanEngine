#include "player.h"

#include "engine.h"
#include "world.h"

void Player::keyboard_control() {
    auto velocity = PLAYER_SPEED * engine.get_delta_time();
    if (engine.get_key_state(GLFW_KEY_W)) move_forward(velocity);
    if (engine.get_key_state(GLFW_KEY_S)) move_back(velocity);
    if (engine.get_key_state(GLFW_KEY_D)) move_right(velocity);
    if (engine.get_key_state(GLFW_KEY_A)) move_left(velocity);
    if (engine.get_key_state(GLFW_KEY_Q)) move_up(velocity);
    if (engine.get_key_state(GLFW_KEY_E)) move_down(velocity);
}

void Player::mouse_control() {
    if (engine.mouse_dx) rotate_yaw(engine.mouse_dx * MOUSE_SENSITIVITY);
    if (engine.mouse_dy) rotate_pitch(engine.mouse_dy * MOUSE_SENSITIVITY);
}
