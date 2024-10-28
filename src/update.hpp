#pragma once

#include <random>

#include "renderer/vulkan_tools.hpp"
#include "particle.hpp"
#include "uniformbuffers.hpp"

const float VELOCITY_FACTOR = 0.0001f;

inline void updateGraphics(void* mapping, float deltatime, uint32_t width, uint32_t height) {
    GraphicsUniformBufferObject ubo = {};
    ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    memcpy(mapping, &ubo, sizeof(ubo));
}

inline void updateCompute(void* mapping, float deltatime, uint32_t width, uint32_t height) {
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();

    // Gets the time from the first call of updateUniformBuffer
    float timeElapsed = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    ComputeUniformBufferObject ubo{};
    ubo.deltaTime = deltatime * 2000.0f;

    glm::vec4 gravityPoint = glm::vec4(0.5f, 0.0f, 0.0f, 1.0f);
    glm::vec3 rotationAxis = glm::vec3(0.1f, 0.1f, 1.0f);

    // Angle dependent on delta time results in cool looking results, but definitely not advised
    float angle = glm::radians(90.0f) * ubo.deltaTime; // timeElapsed

    gravityPoint = gravityPoint * glm::rotate(glm::mat4(1.0f), angle, rotationAxis);
    ubo.gravityPoint = gravityPoint;

    memcpy(mapping, &ubo, sizeof(ubo));
}

inline void initCompute(void* mapping, uint32_t particleCount, uint32_t width, uint32_t height) {
    // Initialize particles
    Particle* particleBuffer = static_cast<Particle*>(mapping);
    std::default_random_engine rndEngine((unsigned)time(nullptr));
    std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

    // Initial particle positions on a circle
    for (size_t i = 0; i < particleCount; i++) {
        float radius = 0.25f * sqrt(rndDist(rndEngine));

        float u = rndDist(rndEngine);
        float v = rndDist(rndEngine);

        float theta = 2.0f * 3.14159265358979323846f * u;
        float phi = acos(2 * v - 1);
        float x = (radius * sin(phi) * cos(theta));
        float y = (radius * sin(phi) * sin(theta));
        float z = (radius * cos(phi));

        particleBuffer[i].position = glm::vec4(x, y, z, 1);
        particleBuffer[i].velocity = glm::vec4(glm::normalize(glm::vec3(x, y, z)) * VELOCITY_FACTOR, 0);
        //        particle.color = glm::vec3(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine));
        particleBuffer[i].color = glm::vec4(0.0f, 100, 100, 1) / 255.0f;
    }
}