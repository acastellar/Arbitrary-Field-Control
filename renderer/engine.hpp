#pragma once

#include "vulkan_tools.hpp"

#include "window.hpp"
#include "physicaldevice.hpp"
#include "swapchain.hpp"
#include "pipeline.hpp"
#include "buffer.hpp"

typedef void (*graphicsUpdateCallback)(void *uniformBuffer, float deltaTime, uint32_t width, uint32_t height);
typedef void (*computeUpdateCallback)(void *uniformBuffer, float deltaTime, uint32_t width, uint32_t height);
typedef void (*computeInitCallback)(void *storageBuffer, uint32_t particleCount, uint32_t width, uint32_t height);

struct EngineConfiguration {
    std::string name;

    int maxFramesInFlight = 3;
    int forcedPresentMode = -1;

    std::string particleFragmentShaderName;
    std::string particleVertexShaderName;
    std::string particleComputeShaderName;
    std::string triangleFragmentShaderName;
    std::string triangleVertexShaderName;

    graphicsUpdateCallback graphicsUpdateCallback;
    computeUpdateCallback computeUpdateCallback;
    computeInitCallback computeInitCallback;

    size_t computeUniformBufferSize = 0;
    size_t graphicsUniformBufferSize = 0;

    uint32_t particleCount = 0;
    size_t particleMemorySize = 0;
    VkVertexInputBindingDescription particleBindingDescription;
    std::vector<VkVertexInputAttributeDescription> particleAttributeDescriptions;
};

class VulkanEngine {
private:
    EngineConfiguration _configuration;

    Window* _window;
    VkDevice _device;
    VkInstance _instance;
    VkSurfaceKHR _surface;

    PhysicalDevice* _physicaldevice;
    GraphicsPipeline* _graphicspipeline;
    ComputePipeline* _computepipeline;
    SwapChain* _swapchain;

    VkQueue _graphicsqueue;
    VkQueue _computequeue;
    VkQueue _presentqueue;

    Buffer* _vertexbuffer;
    Buffer* _indexbuffer;
    std::vector<Buffer*> _graphicsuniformbuffers;
    std::vector<Buffer*> _computeuniformbuffers;
    std::vector<Buffer*> _storagebuffers;

    VkDescriptorPool _graphicsdescriptorpool;
    VkDescriptorPool _computedescriptorpool;
    std::vector<VkDescriptorSet> _graphicsdescriptorsets;
    std::vector<VkDescriptorSet> _computedescriptorsets;

    VkCommandPool _commandpool;
    std::vector<VkCommandBuffer> _graphicscommandbuffers;
    std::vector<VkCommandBuffer> _computecommandbuffers;

    std::vector<VkSemaphore> _imageAvailableSemaphores;
    std::vector<VkSemaphore> _renderFinishedSemaphores;
    std::vector<VkFence> _inFlightFences;

    std::vector<VkSemaphore> _computeFinishedSemaphores;
    std::vector<VkFence> _computeInFlightFences;

    bool _initialized = false;
    bool _framebufferResized = false;
    uint32_t _currentframe = 0;

    float _lastframetime = 0.0f;
    double _lasttime = 0.0;

    std::vector<Vertex> _vertices;
    std::vector<uint32_t> _indices;

    std::optional<VkPresentModeKHR> _forcedpresentmode;

    void initVulkanInstance();
    void selectPhysicalDevice();
    void initLogicalDevice();

    void initGraphicsPipeline();
    void initComputePipeline();
    void initSwapchain();

    void initCommandPool();

    void initGraphicsDescriptorPool();
    void initGraphicsDescriptorSets();

    void initComputeDescriptorPool();
    void initComputeDescriptorSets();

    void initGraphicsCommandBuffers();
    void initComputeCommandBuffers();

    void initSyncObjects();

    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffers();
    void createStorageBuffers();

    void recordComputeCommandBuffer(VkCommandBuffer commandBuffer);
    void recordGraphicsCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void updateGraphicsUniformBuffer(uint32_t currentImage);
    void updateComputeUniformBuffer(uint32_t currentImage);
    void recreateSwapChain();

    void deduplicateVertices();
public:
    VulkanEngine(const EngineConfiguration& configuration);
    ~VulkanEngine();


    void init();
    void draw();

    void setMesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices);

    int windowShouldClose();

    void framebufferResizedCallback();
};
