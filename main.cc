#include "engine.h"
#include "meshes/cloud_mesh.h"
#include "meshes/voxel_marker.h"
#include "meshes/water_mesh.h"

int main(int argc, char* argv[]) {
    Engine engine;

    engine.add_mesh(std::make_unique<CloudMesh>(&engine));
    engine.add_mesh(std::make_unique<WaterMesh>(&engine));
    engine.add_mesh(std::make_unique<VoxelMarkerMesh>(&engine));

    engine.init();
    engine.run();

    return 0;
}
