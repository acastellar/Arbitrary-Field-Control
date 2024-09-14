#include "buffer.hpp"

void Buffer::create(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = usage;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult create_vertex_buffer_result = vkCreateBuffer(_device, &bufferCreateInfo, nullptr, &buffer);
    if (create_vertex_buffer_result != VK_SUCCESS) {
        throw vulkan_error("Failed to create vertex buffer!", create_vertex_buffer_result);
    }

    VkMemoryRequirements bufferMemoryRequirements = {};
    vkGetBufferMemoryRequirements(_device, buffer, &bufferMemoryRequirements);

    VkMemoryAllocateInfo bufferMemoryAllocationInfo = {};
    bufferMemoryAllocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    bufferMemoryAllocationInfo.allocationSize = bufferMemoryRequirements.size;
    bufferMemoryAllocationInfo.memoryTypeIndex = _physicaldevice->findMemoryType(bufferMemoryRequirements.memoryTypeBits,
                                                                properties);

    VkResult buffer_memory_allocation_result = vkAllocateMemory(_device, &bufferMemoryAllocationInfo, nullptr, &memory);
    if (buffer_memory_allocation_result != VK_SUCCESS) {
        throw vulkan_error("Failed to allocate buffer memory!", buffer_memory_allocation_result);
    }

    VkResult vertex_memory_bind_result = vkBindBufferMemory(_device, buffer, memory, 0);
    if (vertex_memory_bind_result != VK_SUCCESS) {
        throw vulkan_error("Failed to bind buffer memory to buffer!", vertex_memory_bind_result);
    }
}

void Buffer::copy(VkBuffer targetBuffer, VkDeviceSize size, VkCommandPool commandPool, VkQueue copyQueue) {
    VkCommandBufferAllocateInfo allocationInfo{};
    allocationInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocationInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocationInfo.commandPool = commandPool;
    allocationInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(_device, &allocationInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, buffer, targetBuffer, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(copyQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(copyQueue);

    vkFreeCommandBuffers(_device, commandPool, 1, &commandBuffer);
}


void Buffer::createOnDevice(VkDeviceSize size, void* data, VkBufferUsageFlags usage, VkCommandPool commandPool, VkQueue copyQueue) {

    Buffer stagingBuffer(_device, _physicaldevice);
    stagingBuffer.create(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    vkMapMemory(_device, stagingBuffer.memory, 0, size, 0, &mapping);
    memcpy(mapping, data, (size_t) size);
    vkUnmapMemory(_device, stagingBuffer.memory);
    mapping = nullptr;

    this->create(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    stagingBuffer.copy(buffer, size, commandPool, copyQueue);
}

void Buffer::createOnHost(VkDeviceSize size, VkBufferUsageFlags usage) {
    this->create(size, usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    vkMapMemory(_device, memory, 0, size, 0, &mapping);
}


Buffer::Buffer(VkDevice device, PhysicalDevice* physicalDevice)
: _device(device), _physicaldevice(physicalDevice), mapping(nullptr), buffer(nullptr), memory(nullptr) {}

Buffer::~Buffer() {
    vkDestroyBuffer(_device, buffer, nullptr);
    vkFreeMemory(_device, memory, nullptr);
}