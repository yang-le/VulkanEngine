# Vulkan Engine

This project is inspired by

- [Voxel Engine (like Minecraft)](https://github.com/StanislavPetrovV/Minecraft)
    for the craft example and the whole framework
- [Vulkan-Hpp](https://github.com/KhronosGroup/Vulkan-Hpp)
    for vulkan interface related codes
- [Vulkan](https://github.com/SaschaWillems/Vulkan)
    for subpasses and many other technics
- [Vulkan-Samples](https://github.com/KhronosGroup/Vulkan-Samples)
    for mipmap generation on the fly and many other technics
- [Vulkan-glTF-PBR](https://github.com/SaschaWillems/Vulkan-glTF-PBR)
    for glTF loading / rendering using vulkan

## Build

The source codes can build using Clang or MSVC. The minimal C++ standard required is C++14. Maybe you can try C++11 but I have not tested.

CMake is used to generate a Ninja or MSVC solution, with vcpkg to fetch third party dependences.

## Third Party Resource

This project depend on the following third party source codes.

- [glfw3](https://github.com/glfw/glfw)
    for window, keyboard and mouse
- [glm](https://github.com/g-truc/glm)
    for vectors, matrix and transformations
- [glslang](https://github.com/KhronosGroup/glslang/tree/main/glslang)
    for compiling shaders to SPIRV and shader include support (DirStackFileIncluder.h)
- [vulkan-headers](https://github.com/KhronosGroup/Vulkan-Headers)
    of course, any other choice available?
- [vulkan-memory-allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
    for memory allocation on GPU
- [imgui](https://github.com/ocornut/imgui)
    for GUIs
- [tingltf](https://github.com/syoyo/tinygltf)
    for loading the glTF model file
- [stb](https://github.com/nothings/stb)
    for loading image file, also used by imgui & tinygltf

They may also depend on other third party source codes.

Also see README.md in the assets and shaders folder.

---

Happy coding. Fire an issue if you find something I missed to mention on the ideas or resources.
