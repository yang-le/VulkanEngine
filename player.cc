#include "player.h"

#include "engine.h"
#include "world.h"

void Player::keyboard_control() {
    auto velocity = PLAYER_SPEED * engine->dt * 1000;  // ms
    if (engine->key_state[GLFW_KEY_W]) move_forward(velocity);
    if (engine->key_state[GLFW_KEY_S]) move_back(velocity);
    if (engine->key_state[GLFW_KEY_D]) move_right(velocity);
    if (engine->key_state[GLFW_KEY_A]) move_left(velocity);
    if (engine->key_state[GLFW_KEY_Q]) move_up(velocity);
    if (engine->key_state[GLFW_KEY_E]) move_down(velocity);
}

void Player::handle_events(int button, int action) {
    if (action == GLFW_PRESS) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            engine->scene.world->voxel_handler->set_voxel();
        } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            engine->scene.world->voxel_handler->switch_mode();
        }
    }
}

void Player::mouse_control() {
    if (engine->dx) rotate_yaw(engine->dx * MOUSE_SENSITIVITY);
    if (engine->dy) rotate_pitch(engine->dy * MOUSE_SENSITIVITY);
}
