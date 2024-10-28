#include <chrono>
#include <thread>

#include "renderer/engine.hpp"
#include "particle.hpp"
#include "uniformbuffers.hpp"
#include "update.hpp"

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

const bool forceLinuxPresentMode = true;

EngineConfiguration getEngineConfiguration() {
    EngineConfiguration configuration = {};
    configuration.name = std::string("Arbitrary Field Control");

    configuration.maxFramesInFlight = 3;
    configuration.forcedPresentMode = -1;

    configuration.computeUniformBufferSize = sizeof(GraphicsUniformBufferObject);
    configuration.graphicsUniformBufferSize = sizeof(GraphicsUniformBufferObject);

    configuration.graphicsUpdateCallback = &updateGraphics;
    configuration.computeUpdateCallback = &updateCompute;
    configuration.computeInitCallback = &initCompute;

    configuration.particleCount = (int) (1000000 / 256) * (256);
    configuration.particleMemorySize = sizeof(Particle);
    configuration.particleBindingDescription = Particle::getBindingDescription();
    configuration.particleAttributeDescriptions = Particle::getAttributeDescriptions();

    configuration.particleVertexShaderName = "shader.vert";
    configuration.particleFragmentShaderName = "shader.frag";
    configuration.triangleVertexShaderName = "shader.particle.vert";
    configuration.triangleFragmentShaderName = "shader.particle.frag";
    configuration.particleComputeShaderName = "shader.comp";

#ifdef __linux__
    if (forceLinuxPresentMode) {
        // Forcing mailbox present mode due to nvidia linux driver bug
        const int MAILBOX_PRESENT_MODE = 1;
        configuration.forcedPresentMode = MAILBOX_PRESENT_MODE;
    }
#endif

    return configuration;
}

int main() {
    const EngineConfiguration engineConfiguration = getEngineConfiguration();

    VulkanEngine renderer(engineConfiguration);
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
