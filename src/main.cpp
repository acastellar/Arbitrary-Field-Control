#include "renderer/engine.hpp"

#include <chrono>
#include <thread>

std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}},

        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}},
};

std::vector<uint32_t> indices = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4
};

int main() {
    // Forcing mailbox present mode due to nvidia linux driver bug
//    const int MAILBOX_PRESENT_MODE = 1;

    RenderingEngine renderer = RenderingEngine("Arbitrary Field Control");
//    renderer.setMesh(vertices, indices);
    renderer.setMesh({{{0,0,0}, {0,0,0}}}, {0,1,2});

    renderer.init();

    auto timeStart = std::chrono::high_resolution_clock::now();

    size_t frame = 0;
    while (!renderer.windowShouldClose()) {
        renderer.draw();
//        renderer.setMesh(vertices, indices);

        frame++;
    }

    auto timeNow = std::chrono::high_resolution_clock::now();
    double timeDifference = std::chrono::duration<double, std::milli>(timeNow - timeStart).count();
    printf("Average framerate: %f\n", frame / (timeDifference * 0.001));

    return 0;
}
