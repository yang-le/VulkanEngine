#include <iostream>

#include "engine.h"
#include "meshes/cloud_mesh.h"
#include "meshes/water_mesh.h"

int main(int argc, char* argv[]) {
    try {
        Engine engine;

        engine.add_mesh(std::make_unique<CloudMesh>(&engine));
        engine.add_mesh(std::make_unique<WaterMesh>(&engine));

        engine.init();
        engine.run();
    } catch (const std::exception& e) {
        std::cerr << e.what();
    } catch (...) {
        std::cerr << "Unknow exception!";
    }

    return 0;
}
