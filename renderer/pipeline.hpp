#pragma once

#include "vulkan_tools.hpp"

const std::string SHADER_FOLDER_PATH = "../shaders/compiled/";
const std::string SHADER_EXTENSION = ".spv";

struct PerspectiveUniformBufferObject {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

struct ModelUniformBufferObject {
    alignas(16) glm::mat4 model;
};

struct Vertex {
    alignas(16) glm::vec3 pos;
    alignas(16) glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
    bool operator==(const Vertex& other) const;
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^
                   (hash<glm::vec3>()(vertex.color) << 1)) >> 1);
        }
    };
}

class GraphicsPipeline {
private:
    void initRenderPass();
    void initDescriptorSetLayout();
    void initLayout();
    void initPipeline(VkShaderModule vertexShader, VkShaderModule fragmentShader, VkShaderModule particleVertexShader, VkShaderModule particleFragmentShader);
    VkDevice _device;
    VkFormat _format;
    VkFormat _depthformat;
    VkSampleCountFlagBits _msaasamples;

    std::string _vertshadername;
    std::string _fragshadername;
    std::string _vertparticleshadername;
    std::string _fragparticleshadername;
public:
    VkPipeline pipeline;
    VkPipeline particlepipeline;
    VkRenderPass renderpass;
    VkPipelineLayout layout;
    VkDescriptorSetLayout descriptorsetlayout;

    void create();

    ~GraphicsPipeline();

    GraphicsPipeline(VkDevice device, VkFormat swapchainFormat, VkFormat depthFormat, VkSampleCountFlagBits msaaSamples,
                     std::string vertexShaderFilename, std::string fragmentShaderFilename,
                     std::string particleVertexShaderFilename, std::string particleFragmentShaderFilename);
};

struct ComputeUniformBufferObject {
    alignas(16) glm::vec4 gravityPoint;
    alignas(16) float deltaTime;
};

struct Particle {
    alignas(16) glm::vec3 position;
    alignas(16) glm::vec3 velocity;
    alignas(16) glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
};

class ComputePipeline {
private:
    void initDescriptorSetLayout();
    void initLayout();
    void initPipeline(VkShaderModule computeShader);

    VkDevice _device;

    std::string _computeshadername;
public:
    VkPipeline pipeline;
    VkPipelineLayout layout;
    VkDescriptorSetLayout descriptorsetlayout;

    void create();

    ~ComputePipeline();
    ComputePipeline(VkDevice device, std::string computeShaderFilename);
};