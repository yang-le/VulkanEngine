#pragma once

#include "shader.h"

struct Engine;
struct VoxelMarkerMesh : Shader {
    VoxelMarkerMesh(Engine* engine);

    virtual void init() override;
    virtual void update() override;
};
