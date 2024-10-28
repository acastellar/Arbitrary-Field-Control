#pragma once

#include <renderer/vulkan_tools.hpp>

struct GraphicsUniformBufferObject {
    alignas(64) glm::mat4 model;
};

struct ComputeUniformBufferObject {
    alignas(16) glm::vec4 gravityPoint;
    alignas(16) float deltaTime;
};

