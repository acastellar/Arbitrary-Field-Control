#pragma once

#include "vulkan_tools.hpp"
#include "physicaldevice.hpp"

class Buffer {
private:
    VkDevice _device;
    PhysicalDevice* _physicaldevice;

    void create(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    void copy(VkBuffer targetBuffer, VkDeviceSize size, VkCommandPool commandPool, VkQueue queue);
public:
    VkBuffer buffer;
    VkDeviceMemory memory;
    void* mapping;

    void createOnDevice(VkDeviceSize size, void* data, VkBufferUsageFlags usage, VkCommandPool commandPool, VkQueue copyQueue);
    void createOnHost(VkDeviceSize size, VkBufferUsageFlags usage);

    Buffer(VkDevice device, PhysicalDevice* physicalDevice);
    ~Buffer();
};
