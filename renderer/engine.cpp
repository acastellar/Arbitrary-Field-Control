#include "engine.hpp"

#include <string>
#include <vector>
#include <set>
#include <optional>
#include <cstring>
#include <fstream>
#include <random>

using std::string, std::vector;
using std::set, std::byte;

#ifdef NDEBUG
    static const bool enableValidationLayers = false;
#else
    static const bool enableValidationLayers = true;
#endif

static const vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
};

static bool checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName: validationLayers) {
        bool layerFound = false;

        for (const VkLayerProperties& layerProperties: availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

static void registerWindowResizedCallback(void* selfpointer, int width, int height) {
    VulkanEngine* renderer = reinterpret_cast<VulkanEngine*>(selfpointer);
    if (renderer) {
        renderer->framebufferResizedCallback();
    }
}

void VulkanEngine::framebufferResizedCallback() {
    _framebufferResized = true;
    // Recreates the framebuffer
    draw();
    // Redraws
    draw();
}

VulkanEngine::VulkanEngine(const EngineConfiguration& configuration) {
    this->_configuration = configuration;

    if (_configuration.forcedPresentMode >= 0) {
        _forcedpresentmode = static_cast<VkPresentModeKHR>(_configuration.forcedPresentMode);
    }

    _device = nullptr;
    _instance = nullptr;
    _surface = nullptr;

    _physicaldevice = nullptr;
    _computepipeline = nullptr;
    _graphicspipeline = nullptr;
    _swapchain = nullptr;

    _graphicsqueue = nullptr;
    _presentqueue = nullptr;

    _commandpool = nullptr;
    _graphicsdescriptorpool = nullptr;

    _vertexbuffer = nullptr;
    _indexbuffer = nullptr;
}

void VulkanEngine::init() {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("Failed to enable vulkan validation layers!");
    }

    _window = new Window(_configuration.name);
    _window->init();

    initVulkanInstance();

    _window->createVulkanSurface(_instance, _surface);

    selectPhysicalDevice();
    initLogicalDevice();

    initGraphicsPipeline();
    initComputePipeline();
    initSwapchain();

    initCommandPool();

    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createStorageBuffers();

    initGraphicsDescriptorPool();
    initGraphicsDescriptorSets();

    initComputeDescriptorPool();
    initComputeDescriptorSets();

    initGraphicsCommandBuffers();
    initComputeCommandBuffers();

    initSyncObjects();

    _initialized = true;
}

void VulkanEngine::initVulkanInstance() {
    VkApplicationInfo applicationInfo = {};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pApplicationName = _configuration.name.c_str();

    VkInstanceCreateInfo instanceCreateInfo = _window->getVulkanInstanceCreateInfo();
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;

#ifdef __APPLE__
    std::vector<const char*> requiredExtensionsMoltenVK;

    for(uint32_t i = 0; i < instanceCreateInfo.enabledExtensionCount; i++) {
        requiredExtensionsMoltenVK.emplace_back(instanceCreateInfo.ppEnabledExtensionNames[i]);
    }
    requiredExtensionsMoltenVK.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    requiredExtensionsMoltenVK.emplace_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);


    instanceCreateInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

    instanceCreateInfo.enabledExtensionCount = (uint32_t) requiredExtensionsMoltenVK.size();
    instanceCreateInfo.ppEnabledExtensionNames = requiredExtensionsMoltenVK.data();
#endif

    if (enableValidationLayers) {
        instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        instanceCreateInfo.enabledLayerCount = 0;
    }

    VkResult instance_creation_result = vkCreateInstance(&instanceCreateInfo, nullptr, &_instance);
    if (instance_creation_result != VK_SUCCESS) {
        throw vulkan_error("Failed to create vulkan instance!", instance_creation_result);
    }
}

void VulkanEngine::selectPhysicalDevice() {
    uint32_t physicalDeviceCount;
    vkEnumeratePhysicalDevices(_instance, &physicalDeviceCount, nullptr);
    if (physicalDeviceCount == 0) {
        throw std::runtime_error("No graphics devices with vulkan support detected!");
    }

    vector<VkPhysicalDevice> vulkanPhysicalDevices(physicalDeviceCount);
    vkEnumeratePhysicalDevices(_instance, &physicalDeviceCount, vulkanPhysicalDevices.data());

    int bestScore = -1;
    for (const VkPhysicalDevice& vulkanPhysicalDevice: vulkanPhysicalDevices) {
        PhysicalDevice* physicalDevice = new PhysicalDevice(vulkanPhysicalDevice);
        physicalDevice->evaluate(_surface);
        if (physicalDevice->score > bestScore) {
            bestScore = physicalDevice->score;
            _physicaldevice = physicalDevice;
        } else {
            delete physicalDevice;
        }
    }

    if (bestScore == -1) {
        throw std::runtime_error("Failed to find a suitable graphics device!");
    }

    _physicaldevice->forcePresentMode(_forcedpresentmode);
}

void VulkanEngine::initLogicalDevice() {
    const QueueFamilyIndices& queueFamilyIndices = _physicaldevice->queuefamilies;

    float deviceQueuePriority = 1.0f;
    std::set<uint32_t> uniqueQueueFamilies = {queueFamilyIndices.graphicsComputeFamily.value(),
                                         queueFamilyIndices.presentFamily.value()};

    vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos;
    for (uint32_t queueFamily: uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo deviceQueueCreateInfo = {};
        deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        deviceQueueCreateInfo.queueFamilyIndex = queueFamily;
        deviceQueueCreateInfo.queueCount = 1;
        deviceQueueCreateInfo.pQueuePriorities = &deviceQueuePriority;

        deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.sampleRateShading = VK_TRUE;

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos.data();
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(deviceQueueCreateInfos.size());

    if (enableValidationLayers) {
        deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        deviceCreateInfo.enabledLayerCount = 0;
    }

    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size());
    deviceCreateInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();

    VkResult device_creation_result = vkCreateDevice(_physicaldevice->physicaldevice, &deviceCreateInfo, nullptr, &_device);
    if (device_creation_result != VK_SUCCESS) {
        throw vulkan_error("Failed to create logical device!", device_creation_result);
    }

    vkGetDeviceQueue(_device, queueFamilyIndices.graphicsComputeFamily.value(), 0, &_graphicsqueue);
    vkGetDeviceQueue(_device, queueFamilyIndices.graphicsComputeFamily.value(), 0, &_computequeue);
    vkGetDeviceQueue(_device, queueFamilyIndices.presentFamily.value(), 0, &_presentqueue);
}

void VulkanEngine::initGraphicsPipeline() {
    _graphicspipeline = new GraphicsPipeline(_device,
                                             _physicaldevice->swapsurfaceformat.format,
                                             _physicaldevice->findDepthFormat(),
                                             _physicaldevice->msaasamples,
                                             _configuration.particleBindingDescription,
                                             _configuration.particleAttributeDescriptions,
                                             _configuration.triangleVertexShaderName,
                                             _configuration.triangleFragmentShaderName,
                                             _configuration.particleVertexShaderName,
                                             _configuration.particleFragmentShaderName);
    _graphicspipeline->create();
}

void VulkanEngine::initComputePipeline() {
    _computepipeline = new ComputePipeline(_device, "shader.comp");
    _computepipeline->create();
}

void VulkanEngine::initSwapchain() {
    int framebufferwidth, framebufferheight;
    _window->getSizePixels(framebufferwidth, framebufferheight);

    _swapchain = new SwapChain(_device, _surface, _physicaldevice);
    _swapchain->create(_graphicspipeline->renderpass, framebufferwidth, framebufferheight);

    _window->setResizeCallback(this, registerWindowResizedCallback);
}

void VulkanEngine::initCommandPool() {
    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCreateInfo.queueFamilyIndex = _physicaldevice->queuefamilies.graphicsComputeFamily.value();

    VkResult command_pool_creation_result = vkCreateCommandPool(_device, &commandPoolCreateInfo, nullptr, &_commandpool);
    if (command_pool_creation_result != VK_SUCCESS) {
        throw vulkan_error("Failed to create command pool!", command_pool_creation_result);
    }
}

void VulkanEngine::createVertexBuffer() {
    VkDeviceSize vertexBufferSize = sizeof(_vertices[0]) * _vertices.size();
    _vertexbuffer = new Buffer(_device, _physicaldevice);
    _vertexbuffer->createOnDevice(vertexBufferSize, (void*) _vertices.data(),
                                  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                 _commandpool,_graphicsqueue);
}

void VulkanEngine::createIndexBuffer() {
    VkDeviceSize indexBufferSize = sizeof(_indices[0]) * _indices.size();
    _indexbuffer = new Buffer(_device, _physicaldevice);
    _indexbuffer->createOnDevice(indexBufferSize, (void*) _indices.data(),
                                 VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                 _commandpool,_graphicsqueue);
}

void VulkanEngine::createUniformBuffers() {
    VkDeviceSize graphicsUniformBufferSize = sizeof(PerspectiveUniformBufferObject) + _configuration.graphicsUniformBufferSize;
    _graphicsuniformbuffers.resize(_configuration.maxFramesInFlight);
    for (size_t i = 0; i < _configuration.maxFramesInFlight; i++) {
        _graphicsuniformbuffers[i] = new Buffer(_device, _physicaldevice);
        _graphicsuniformbuffers[i]->createOnHost(graphicsUniformBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    }

    VkDeviceSize computeUniformBufferSize = _configuration.graphicsUniformBufferSize;
    _computeuniformbuffers.resize(_configuration.maxFramesInFlight);
    for (size_t i = 0; i < _configuration.maxFramesInFlight; i++) {
        _computeuniformbuffers[i] = new Buffer(_device, _physicaldevice);
        _computeuniformbuffers[i]->createOnHost(computeUniformBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    }
}

void VulkanEngine::createStorageBuffers() {

    VkDeviceSize bufferSize = _configuration.particleMemorySize * _configuration.particleCount;
    void* bufferData = malloc(bufferSize);

    _configuration.computeInitCallback(bufferData, _configuration.particleCount,
                                       _swapchain->extent.width, _swapchain->extent.height);

    _storagebuffers.resize(_configuration.maxFramesInFlight);

    for (size_t i = 0; i < _configuration.maxFramesInFlight; i++) {
        _storagebuffers[i] = new Buffer(_device, _physicaldevice);
        _storagebuffers[i]->createOnDevice(bufferSize, bufferData,
                                           VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                           _commandpool, _computequeue);
    }

    free(bufferData);
}

void VulkanEngine::initGraphicsDescriptorPool() {
    std::array<VkDescriptorPoolSize, 1> descriptorPoolSizes = {};
    descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorPoolSizes[0].descriptorCount = 2 * static_cast<uint32_t>(_configuration.maxFramesInFlight);

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());
    descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes.data();
    descriptorPoolCreateInfo.maxSets = static_cast<uint32_t>(_configuration.maxFramesInFlight);

    VkResult descriptor_pool_creation_result = vkCreateDescriptorPool(_device, &descriptorPoolCreateInfo,
                                                                      nullptr, &_graphicsdescriptorpool);
    if (descriptor_pool_creation_result != VK_SUCCESS) {
        throw vulkan_error("Failed to creator descriptor pool!", descriptor_pool_creation_result);
    }
}


void VulkanEngine::initGraphicsDescriptorSets() {
    vector<VkDescriptorSetLayout> descriptorSetLayouts(_configuration.maxFramesInFlight, _graphicspipeline->descriptorsetlayout);
    VkDescriptorSetAllocateInfo descriptorSetAllocationInfo = {};
    descriptorSetAllocationInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocationInfo.descriptorPool = _graphicsdescriptorpool;
    descriptorSetAllocationInfo.descriptorSetCount = static_cast<uint32_t>(_configuration.maxFramesInFlight);
    descriptorSetAllocationInfo.pSetLayouts = descriptorSetLayouts.data();

    _graphicsdescriptorsets.resize(_configuration.maxFramesInFlight);
    VkResult descriptor_sets_allocation_result = vkAllocateDescriptorSets(_device, &descriptorSetAllocationInfo,
                                                                          _graphicsdescriptorsets.data());
    if (descriptor_sets_allocation_result != VK_SUCCESS) {
        throw vulkan_error("Failed to allocate descriptor sets!", descriptor_sets_allocation_result);
    }

    for (size_t i = 0; i < _configuration.maxFramesInFlight; i++) {

        std::array<VkWriteDescriptorSet, 2> writeDescriptorSets = {};

        VkDescriptorBufferInfo perspectiveUniformBufferInfo;
        perspectiveUniformBufferInfo.buffer = _graphicsuniformbuffers[i]->buffer;
        perspectiveUniformBufferInfo.offset = 0;
        perspectiveUniformBufferInfo.range = sizeof(PerspectiveUniformBufferObject);

        writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[0].dstSet = _graphicsdescriptorsets[i];
        writeDescriptorSets[0].dstBinding = 0;
        writeDescriptorSets[0].dstArrayElement = 0;
        writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSets[0].descriptorCount = 1;
        writeDescriptorSets[0].pBufferInfo = &perspectiveUniformBufferInfo;

        VkDescriptorBufferInfo modelUniformBufferInfo;
        modelUniformBufferInfo.buffer = _graphicsuniformbuffers[i]->buffer;
        modelUniformBufferInfo.offset = sizeof(PerspectiveUniformBufferObject);
        modelUniformBufferInfo.range = _configuration.graphicsUniformBufferSize;

        writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[1].dstSet = _graphicsdescriptorsets[i];
        writeDescriptorSets[1].dstBinding = 1;
        writeDescriptorSets[1].dstArrayElement = 0;
        writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSets[1].descriptorCount = 1;
        writeDescriptorSets[1].pBufferInfo = &modelUniformBufferInfo;


        vkUpdateDescriptorSets(_device, static_cast<uint32_t>(writeDescriptorSets.size()),
                               writeDescriptorSets.data(), 0, nullptr);
    }
}

void VulkanEngine::initComputeDescriptorPool() {
    std::array<VkDescriptorPoolSize, 2> descriptorPoolSizes = {};
    descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorPoolSizes[0].descriptorCount = static_cast<uint32_t>(_configuration.maxFramesInFlight);

    descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorPoolSizes[1].descriptorCount = 2 * static_cast<uint32_t>(_configuration.maxFramesInFlight);

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());
    descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes.data();
    descriptorPoolCreateInfo.maxSets = static_cast<uint32_t>(_configuration.maxFramesInFlight);

    VkResult descriptor_pool_creation_result = vkCreateDescriptorPool(_device, &descriptorPoolCreateInfo,
                                                                      nullptr, &_computedescriptorpool);
    if (descriptor_pool_creation_result != VK_SUCCESS) {
        throw vulkan_error("Failed to creator descriptor pool!", descriptor_pool_creation_result);
    }
}

void VulkanEngine::initComputeDescriptorSets() {
    vector<VkDescriptorSetLayout> descriptorSetLayouts(_configuration.maxFramesInFlight, _computepipeline->descriptorsetlayout);
    VkDescriptorSetAllocateInfo descriptorSetAllocationInfo = {};
    descriptorSetAllocationInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocationInfo.descriptorPool = _computedescriptorpool;
    descriptorSetAllocationInfo.descriptorSetCount = static_cast<uint32_t>(_configuration.maxFramesInFlight);
    descriptorSetAllocationInfo.pSetLayouts = descriptorSetLayouts.data();

    _computedescriptorsets.resize(_configuration.maxFramesInFlight);
    VkResult descriptor_sets_allocation_result = vkAllocateDescriptorSets(_device, &descriptorSetAllocationInfo,
                                                                          _computedescriptorsets.data());
    if (descriptor_sets_allocation_result != VK_SUCCESS) {
        throw vulkan_error("Failed to allocate descriptor sets!", descriptor_sets_allocation_result);
    }

    for (size_t i = 0; i < _configuration.maxFramesInFlight; i++) {

        std::array<VkWriteDescriptorSet, 3> writeDescriptorSets = {};

        VkDescriptorBufferInfo uniformBufferInfo;
        uniformBufferInfo.buffer = _computeuniformbuffers[i]->buffer;
        uniformBufferInfo.offset = 0;
        uniformBufferInfo.range = _configuration.computeUniformBufferSize;

        writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[0].dstSet = _computedescriptorsets[i];
        writeDescriptorSets[0].dstBinding = 0;
        writeDescriptorSets[0].dstArrayElement = 0;
        writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSets[0].descriptorCount = 1;
        writeDescriptorSets[0].pBufferInfo = &uniformBufferInfo;

        VkDescriptorBufferInfo storageBufferInfoPreviousFrame = {};
        storageBufferInfoPreviousFrame.buffer = _storagebuffers[(i - 1) % _configuration.maxFramesInFlight]->buffer;
        storageBufferInfoPreviousFrame.offset = 0;
        storageBufferInfoPreviousFrame.range = _configuration.particleMemorySize * _configuration.particleCount;

        writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[1].dstSet = _computedescriptorsets[i];
        writeDescriptorSets[1].dstBinding = 1;
        writeDescriptorSets[1].dstArrayElement = 0;
        writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writeDescriptorSets[1].descriptorCount = 1;
        writeDescriptorSets[1].pBufferInfo = &storageBufferInfoPreviousFrame;

        VkDescriptorBufferInfo storageBufferInfoCurrentFrame = {};
        storageBufferInfoCurrentFrame.buffer = _storagebuffers[i]->buffer;
        storageBufferInfoCurrentFrame.offset = 0;
        storageBufferInfoCurrentFrame.range = _configuration.particleMemorySize * _configuration.particleCount;

        writeDescriptorSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[2].dstSet = _computedescriptorsets[i];
        writeDescriptorSets[2].dstBinding = 2;
        writeDescriptorSets[2].dstArrayElement = 0;
        writeDescriptorSets[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writeDescriptorSets[2].descriptorCount = 1;
        writeDescriptorSets[2].pBufferInfo = &storageBufferInfoCurrentFrame;

        vkUpdateDescriptorSets(_device, static_cast<uint32_t>(writeDescriptorSets.size()),
                               writeDescriptorSets.data(), 0, nullptr);
    }
}

void VulkanEngine::initGraphicsCommandBuffers() {
    _graphicscommandbuffers.resize(_configuration.maxFramesInFlight);

    VkCommandBufferAllocateInfo allocationInfo = {};
    allocationInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocationInfo.commandPool = _commandpool;
    allocationInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocationInfo.commandBufferCount = (uint32_t) _graphicscommandbuffers.size();

    VkResult command_buffer_creation_result = vkAllocateCommandBuffers(_device, &allocationInfo, _graphicscommandbuffers.data());
    if (command_buffer_creation_result != VK_SUCCESS) {
        throw vulkan_error("Failed to create command buffers", command_buffer_creation_result);
    }
}

void VulkanEngine::initComputeCommandBuffers() {
    _computecommandbuffers.resize(_configuration.maxFramesInFlight);

    VkCommandBufferAllocateInfo allocationInfo = {};
    allocationInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocationInfo.commandPool = _commandpool;
    allocationInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocationInfo.commandBufferCount = (uint32_t) _computecommandbuffers.size();

    VkResult command_buffer_creation_result = vkAllocateCommandBuffers(_device, &allocationInfo, _computecommandbuffers.data());
    if (command_buffer_creation_result != VK_SUCCESS) {
        throw vulkan_error("Failed to create compute command buffers", command_buffer_creation_result);
    }
}

void VulkanEngine::initSyncObjects() {
    _imageAvailableSemaphores.resize(_configuration.maxFramesInFlight);
    _renderFinishedSemaphores.resize(_configuration.maxFramesInFlight);
    _inFlightFences.resize(_configuration.maxFramesInFlight);

    _computeFinishedSemaphores.resize(_configuration.maxFramesInFlight);
    _computeInFlightFences.resize(_configuration.maxFramesInFlight);

    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < _configuration.maxFramesInFlight; i++) {
        VkResult sync_object_creation_result;

        sync_object_creation_result = vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr,
                                                        &_imageAvailableSemaphores[i]);
        if (sync_object_creation_result != VK_SUCCESS) {
            throw vulkan_error("Failed to create image available synchronization semaphore!", sync_object_creation_result);
        }

        sync_object_creation_result = vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr,
                                                        &_renderFinishedSemaphores[i]);
        if (sync_object_creation_result != VK_SUCCESS) {
            throw vulkan_error("Failed to create render finished synchronization semaphore!", sync_object_creation_result);
        }

        sync_object_creation_result = vkCreateFence(_device, &fenceCreateInfo, nullptr,
                                                    &_inFlightFences[i]);
        if (sync_object_creation_result != VK_SUCCESS) {
            throw vulkan_error("Failed to create render in-flight synchronization fence!", sync_object_creation_result);
        }

        sync_object_creation_result = vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr,
                                                        &_computeFinishedSemaphores[i]);
        if (sync_object_creation_result != VK_SUCCESS) {
            throw vulkan_error("Failed to create compute finished synchronization semaphore!", sync_object_creation_result);
        }

        sync_object_creation_result = vkCreateFence(_device, &fenceCreateInfo, nullptr,
                                                    &_computeInFlightFences[i]);
        if (sync_object_creation_result != VK_SUCCESS) {
            throw vulkan_error("Failed to create compute in-flight synchronization fence!", sync_object_creation_result);
        }
    }
}

void VulkanEngine::recordComputeCommandBuffer(VkCommandBuffer commandBuffer) {
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VkResult begin_command_buffer_result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    if (begin_command_buffer_result != VK_SUCCESS) {
        throw vulkan_error("Failed to start recording compute command buffer", begin_command_buffer_result);
    }

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _computepipeline->pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _computepipeline->layout,
                            0, 1, &_computedescriptorsets[_currentframe],
                            0, nullptr);

    vkCmdDispatch(commandBuffer, _configuration.particleCount / 256, 1, 1);


    VkResult command_buffer_end_result = vkEndCommandBuffer(commandBuffer);
    if (command_buffer_end_result != VK_SUCCESS) {
        throw vulkan_error("Failed to finish recording compute command buffer!", command_buffer_end_result);
    }
}

void VulkanEngine::recordGraphicsCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VkResult begin_command_buffer_result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    if (begin_command_buffer_result != VK_SUCCESS) {
        throw vulkan_error("Failed to start recording graphics command buffer", begin_command_buffer_result);
    }

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = _graphicspipeline->renderpass;
    renderPassBeginInfo.framebuffer = _swapchain->framebuffers[imageIndex];
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent = _swapchain->extent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassBeginInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(_swapchain->extent.width);
    viewport.height = static_cast<float>(_swapchain->extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = _swapchain->extent;

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicspipeline->pipeline);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicspipeline->layout,
                            0, 1, &_graphicsdescriptorsets[_currentframe],
                            0, nullptr);

    VkDeviceSize offsets[] = {0};
    VkBuffer vertexBuffers[] = {_vertexbuffer->buffer};

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, _indexbuffer->buffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(_indices.size()), 1, 0, 0, 0);

    // Particles
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicspipeline->particlepipeline);

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &_storagebuffers[_currentframe]->buffer, offsets);

    vkCmdDraw(commandBuffer, _configuration.particleCount, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    VkResult command_buffer_end_result = vkEndCommandBuffer(commandBuffer);
    if (command_buffer_end_result != VK_SUCCESS) {
        throw vulkan_error("Failed to finish recording graphics command buffer!", command_buffer_end_result);
    }
}

void VulkanEngine::updateGraphicsUniformBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();

    // Gets the time from the first call of updateUniformBuffer
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    PerspectiveUniformBufferObject perspectiveUBO = {};
    perspectiveUBO.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    perspectiveUBO.proj = glm::perspective(glm::radians(45.0f), (float) _swapchain->extent.width / (float) _swapchain->extent.height, 0.1f, 10.0f);
    // glm is designed for OpenGL, where the y coordinate is flipped the other way around.
    // We compensate for this by flipping the projection matrix.
    perspectiveUBO.proj[1][1] *= -1;

    memcpy(_graphicsuniformbuffers[currentImage]->mapping, &perspectiveUBO, sizeof(perspectiveUBO));

    size_t graphicsUBOOffset = sizeof(PerspectiveUniformBufferObject);
    void* graphicsUBOMapping = static_cast<std::byte*>(_graphicsuniformbuffers[currentImage]->mapping) + graphicsUBOOffset;

    _configuration.graphicsUpdateCallback(graphicsUBOMapping, _lastframetime,
                                          _swapchain->extent.width, _swapchain->extent.height);
}

void VulkanEngine::updateComputeUniformBuffer(uint32_t currentImage) {
    _configuration.computeUpdateCallback(_computeuniformbuffers[currentImage]->mapping, _lastframetime,
                                         _swapchain->extent.width, _swapchain->extent.height);
}

void VulkanEngine::recreateSwapChain() {
    vkDeviceWaitIdle(_device);

    delete _swapchain;

    int framebufferwidth, framebufferheight;
    _window->getSizePixels(framebufferwidth, framebufferheight);

    _swapchain = new SwapChain(_device, _surface, _physicaldevice);
    _swapchain->create(_graphicspipeline->renderpass, framebufferwidth, framebufferheight);
}


void VulkanEngine::draw() {
    _window->update();

    // Compute //
    vkWaitForFences(_device, 1, &_computeInFlightFences[_currentframe], VK_TRUE, UINT64_MAX);

    updateComputeUniformBuffer(_currentframe);

    vkResetFences(_device, 1, &_computeInFlightFences[_currentframe]);

    vkResetCommandBuffer(_computecommandbuffers[_currentframe], 0);
    recordComputeCommandBuffer(_computecommandbuffers[_currentframe]);

    VkSubmitInfo computeSubmitInfo = {};
    computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    computeSubmitInfo.commandBufferCount = 1;
    computeSubmitInfo.pCommandBuffers = &_computecommandbuffers[_currentframe];
    computeSubmitInfo.signalSemaphoreCount = 1;
    computeSubmitInfo.pSignalSemaphores = &_computeFinishedSemaphores[_currentframe];

    VkResult compute_queue_submit_result = vkQueueSubmit(_computequeue, 1, &computeSubmitInfo, _computeInFlightFences[_currentframe]);
    if (compute_queue_submit_result != VK_SUCCESS) {
        throw vulkan_error("Failed to submit command buffer to compute queue!", compute_queue_submit_result);
    }

    vkWaitForFences(_device, 1, &_inFlightFences[_currentframe], VK_TRUE, UINT32_MAX);

    // Graphics
    uint32_t imageIndex;
    VkResult acquire_image_result = vkAcquireNextImageKHR(_device, _swapchain->swapchain, UINT64_MAX,
                                                          _imageAvailableSemaphores[_currentframe],
                                                          VK_NULL_HANDLE, &imageIndex);

    if (acquire_image_result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    } else if (acquire_image_result != VK_SUCCESS && acquire_image_result != VK_SUBOPTIMAL_KHR) {
        throw vulkan_error("Failed to acquire next swap chain image!", acquire_image_result);
    }

    vkResetFences(_device, 1, &_inFlightFences[_currentframe]);

    updateGraphicsUniformBuffer(_currentframe);

    vkResetCommandBuffer(_graphicscommandbuffers[_currentframe], 0);
    recordGraphicsCommandBuffer(_graphicscommandbuffers[_currentframe], imageIndex);

    VkSubmitInfo graphicsSubmitInfo = {};
    graphicsSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {_computeFinishedSemaphores[_currentframe], _imageAvailableSemaphores[_currentframe]};
    graphicsSubmitInfo.waitSemaphoreCount = 2;
    graphicsSubmitInfo.pWaitSemaphores = waitSemaphores;

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    graphicsSubmitInfo.pWaitDstStageMask = waitStages;

    VkSemaphore signalSemaphores[] = {_renderFinishedSemaphores[_currentframe]};
    graphicsSubmitInfo.signalSemaphoreCount = 1;
    graphicsSubmitInfo.pSignalSemaphores = signalSemaphores;

    graphicsSubmitInfo.commandBufferCount = 1;
    graphicsSubmitInfo.pCommandBuffers = &_graphicscommandbuffers[_currentframe];

    VkResult graphics_queue_submit_result = vkQueueSubmit(_graphicsqueue, 1, &graphicsSubmitInfo, _inFlightFences[_currentframe]);
    if (graphics_queue_submit_result != VK_SUCCESS) {
        throw vulkan_error("Failed to submit command buffer to graphics queue!", graphics_queue_submit_result);
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = {_swapchain->swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    VkResult present_queue_submit_result = vkQueuePresentKHR(_presentqueue, &presentInfo);
    if (present_queue_submit_result == VK_ERROR_OUT_OF_DATE_KHR || present_queue_submit_result == VK_SUBOPTIMAL_KHR || _framebufferResized) {
        _framebufferResized = false;
        recreateSwapChain();
    } else if (present_queue_submit_result != VK_SUCCESS) {
        throw vulkan_error("Failed to submit swap chain image to present queue!", present_queue_submit_result);
    }

    _currentframe = (_currentframe + 1) % _configuration.maxFramesInFlight;

    double currentTime = _window->getWindowTime();
    _lastframetime = ((currentTime - _lasttime));
    _lasttime = currentTime;
}

void VulkanEngine::deduplicateVertices() {
    std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

    std::vector<Vertex> deduplicatedVertices;
    std::vector<uint32_t> deduplicatedIndices;

    for (const auto& index : _indices) {
        const Vertex& vertex = _vertices[index];

        if (uniqueVertices.count(vertex) == 0) {
            uniqueVertices[vertex] = static_cast<uint32_t>(deduplicatedVertices.size());
            deduplicatedVertices.push_back(vertex);
        }

        deduplicatedIndices.push_back(uniqueVertices[vertex]);
    }

    _vertices = std::move(deduplicatedVertices);
    _indices = std::move(deduplicatedIndices);
}

void VulkanEngine::setMesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices) {
    _vertices = vertices;
    _indices = indices;

    deduplicateVertices();

    if (_initialized) {
        // Not great, but I don't feel like caring about performance here too much
        vkDeviceWaitIdle(_device);
        delete _vertexbuffer;
        delete _indexbuffer;
        createVertexBuffer();
        createIndexBuffer();
    }
}

int VulkanEngine::windowShouldClose() {
    return _window->windowShouldClose();
}

VulkanEngine::~VulkanEngine() {
    if (_device) {
        vkDeviceWaitIdle(_device);

        vkDestroyCommandPool(_device, _commandpool, nullptr);
        vkDestroyDescriptorPool(_device, _graphicsdescriptorpool, nullptr);
        vkDestroyDescriptorPool(_device, _computedescriptorpool, nullptr);

        delete _vertexbuffer;
        delete _indexbuffer;

        for (size_t i = 0; i < _configuration.maxFramesInFlight; i++) {
            vkDestroySemaphore(_device, _imageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(_device, _renderFinishedSemaphores[i], nullptr);
            vkDestroyFence(_device, _inFlightFences[i], nullptr);

            vkDestroySemaphore(_device, _computeFinishedSemaphores[i], nullptr);
            vkDestroyFence(_device, _computeInFlightFences[i], nullptr);

            delete _graphicsuniformbuffers[i];
            delete _computeuniformbuffers[i];

            delete _storagebuffers[i];
        }

        delete _graphicspipeline;
        delete _computepipeline;
        delete _swapchain;
    }

    if (_instance && _surface) {
        vkDestroySurfaceKHR(_instance, _surface, nullptr);
    }

    vkDestroyDevice(_device, nullptr);
    vkDestroyInstance(_instance, nullptr);

    delete _window;
}

