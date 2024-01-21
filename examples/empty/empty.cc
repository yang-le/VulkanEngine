#include <iostream>

#include "engine.h"

int main(int argc, char* argv[]) {
    try {
        Engine engine(1600, 900);
        engine.vulkan.addRenderPass();
        engine.run();
    } catch (const std::exception& e) {
        std::cerr << e.what();
    } catch (...) {
        std::cerr << "Unknow exception!";
    }

    return 0;
}
