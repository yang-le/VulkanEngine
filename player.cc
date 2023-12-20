#include "player.h"

#include "engine.h"
#include "world.h"

void Player::keyboard_control() {
    auto ds = PLAYER_SPEED * engine.get_delta_time();
    if (engine.get_key_state(GLFW_KEY_W)) move_forward(ds);
    if (engine.get_key_state(GLFW_KEY_S)) move_back(ds);
    if (engine.get_key_state(GLFW_KEY_D)) move_right(ds);
    if (engine.get_key_state(GLFW_KEY_A)) move_left(ds);
    if (engine.get_key_state(GLFW_KEY_Q)) move_up(ds);
    if (engine.get_key_state(GLFW_KEY_E)) move_down(ds);
}

void Player::mouse_control() {
    if (engine.mouse_dx) rotate_yaw(engine.mouse_dx * MOUSE_SENSITIVITY);
    if (engine.mouse_dy) rotate_pitch(engine.mouse_dy * MOUSE_SENSITIVITY);
}
