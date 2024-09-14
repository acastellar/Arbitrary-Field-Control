#pragma once

#include "vulkan_tools.hpp"

#include "window.hpp"
#include "physicaldevice.hpp"
#include "swapchain.hpp"
#include "pipeline.hpp"
#include "buffer.hpp"

const int MAX_FRAMES_IN_FLIGHT = 3;

const uint32_t PARTICLE_COUNT = (int) (10000000 / 256) * (256);
const float VELOCITY_FACTOR = 0.0001f;

class RenderingEngine {
private:
    std::string _name;

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
    RenderingEngine(std::string name, int forcedPresentMode);
    RenderingEngine(std::string name);
    ~RenderingEngine();


    void init();
    void draw();

    void setMesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices);

    int windowShouldClose();

    void framebufferResized();
};
