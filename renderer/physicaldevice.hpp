#pragma once

#include "vulkan_tools.hpp"

#include <optional>

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsComputeFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const {
        return graphicsComputeFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

const std::vector<const char*> DEVICE_EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

class PhysicalDevice {
private:
    QueueFamilyIndices findQueueFamilies(VkSurfaceKHR surface);
    SwapChainSupportDetails querySwapChainSupportDetails(VkSurfaceKHR surface);

    bool checkDeviceExtensionSupport();
    bool isSwapChainAdequate();

    int rateSuitability();

    VkPresentModeKHR chooseSwapPresentMode();
    VkSurfaceFormatKHR chooseSwapSurfaceFormat();
    VkSampleCountFlagBits getMaxUsableSampleCount();

    std::optional<VkPresentModeKHR> _forcedpresentmode;
public:
    VkPhysicalDevice physicaldevice;
    QueueFamilyIndices queuefamilies;
    SwapChainSupportDetails swapchainsupport;
    VkPresentModeKHR swappresentmode;
    VkSurfaceFormatKHR swapsurfaceformat;
    VkSampleCountFlagBits msaasamples;

    int score;

    void forcePresentMode(std::optional<VkPresentModeKHR> presentMode);

    void evaluate(VkSurfaceKHR surface);

    VkExtent2D getSwapExtent(VkSurfaceKHR surface, int framebufferwidth, int framebufferheight);

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat findDepthFormat();

    PhysicalDevice(VkPhysicalDevice vulkanPhysicalDevice): physicaldevice(vulkanPhysicalDevice) {};

};