#pragma once

#include <glm/glm.hpp>

// resolution
constexpr int WIN_WIDTH = 1600;
constexpr int WIN_HEIGHT = 900;

// world generation
constexpr int SEED = 16;

// ray casting
constexpr int MAX_RAY_DIST = 6;

// chunk
constexpr int CHUNK_SIZE = 48;
constexpr int H_CHUNK_SIZE = CHUNK_SIZE / 2;
constexpr int CHUNK_AREA = CHUNK_SIZE * CHUNK_SIZE;
constexpr int CHUNK_VOL = CHUNK_AREA * CHUNK_SIZE;
constexpr float CHUNK_SPHERE_RADIUS = H_CHUNK_SIZE * 1.732050807569f;

// world
constexpr int WORLD_W = 20, WORLD_H = 2;
constexpr int WORLD_D = WORLD_W;
constexpr int WORLD_AREA = WORLD_W * WORLD_D;
constexpr int WORLD_VOL = WORLD_AREA * WORLD_H;

// world center
constexpr int CENTER_XZ = WORLD_W * H_CHUNK_SIZE;
constexpr int CENTER_Y = WORLD_H * H_CHUNK_SIZE;

// camera
constexpr float ASPECT_RATIO = (float)WIN_WIDTH / WIN_HEIGHT;
constexpr float FOV_DEG = 50;
constexpr float V_FOV = glm::radians(FOV_DEG);  // vertical FOV
constexpr float ZNEAR = 0.1f;
constexpr float ZFAR = 2000.0f;

// player
constexpr glm::vec3 PLAYER_POS = glm::vec3(CENTER_XZ, WORLD_H* CHUNK_SIZE, CENTER_XZ);
constexpr float MOUSE_SENSITIVITY = 0.002f;

// colors
constexpr glm::vec3 BG_COLOR = glm::vec3(0.58, 0.83, 0.99);

// textures
constexpr int SAND = 1;
constexpr int GRASS = 2;
constexpr int DIRT = 3;
constexpr int STONE = 4;
constexpr int SNOW = 5;
constexpr int LEAVES = 6;
constexpr int WOOD = 7;

// terrain levels
constexpr int SNOW_LVL = 54;
constexpr int STONE_LVL = 49;
constexpr int DIRT_LVL = 40;
constexpr int GRASS_LVL = 8;
constexpr int SAND_LVL = 7;

// tree settings
constexpr float TREE_PROBABILITY = 0.02f;
constexpr int TREE_WIDTH = 4, TREE_HEIGHT = 8;
constexpr int TREE_H_WIDTH = TREE_WIDTH / 2, TREE_H_HEIGHT = TREE_HEIGHT / 2;

// water
constexpr int WATER_AREA = 5 * CHUNK_SIZE * WORLD_W;

// cloud
constexpr int CLOUD_SCALE = 25;
constexpr int CLOUD_HEIGHT = WORLD_H * CHUNK_SIZE * 2;
