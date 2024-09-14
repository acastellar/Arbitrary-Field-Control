#include "physicaldevice.hpp"

#include <set>
#include <algorithm>

using std::vector, std::string, std::set;

// Returns queue families that supports necessary queues.
// If no such queue families exists, the indices returned will be incomplete (!isComplete()).
QueueFamilyIndices PhysicalDevice::findQueueFamilies(VkSurfaceKHR surface) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicaldevice, &queueFamilyCount, nullptr);

    vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicaldevice, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const VkQueueFamilyProperties& queueFamily: queueFamilies) {
        // graphics family
        if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
            indices.graphicsComputeFamily = i;
        }

        // presentation (windowing system) family
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicaldevice, i, surface, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
        }


        if (indices.isComplete()) {
            break;
        }

        i++;
    }

    return indices;
}


// Checks if a device has all the "DEVICE_EXTENSIONS" available.
bool PhysicalDevice::checkDeviceExtensionSupport() {
    uint32_t availableExtensionCount;
    vkEnumerateDeviceExtensionProperties(physicaldevice, nullptr, &availableExtensionCount, nullptr);

    vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
    vkEnumerateDeviceExtensionProperties(physicaldevice, nullptr, &availableExtensionCount, availableExtensions.data());

    set<string> requiredExtensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());

    for (const VkExtensionProperties& extensionProperties: availableExtensions) {
        requiredExtensions.erase(extensionProperties.extensionName);
    }

    return requiredExtensions.empty();
}


SwapChainSupportDetails PhysicalDevice::querySwapChainSupportDetails(VkSurfaceKHR surface) {

    SwapChainSupportDetails swapChainSupportDetails;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicaldevice, surface, &swapChainSupportDetails.capabilities);

    uint32_t surfaceFormatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicaldevice, surface, &surfaceFormatCount, nullptr);

    if (surfaceFormatCount > 0)
{
        swapChainSupportDetails.formats.resize(surfaceFormatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicaldevice, surface, &surfaceFormatCount, swapChainSupportDetails.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicaldevice, surface, &presentModeCount, nullptr);

    if (presentModeCount > 0) {
        swapChainSupportDetails.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicaldevice, surface, &presentModeCount, swapChainSupportDetails.presentModes.data());
    }

    return swapChainSupportDetails;
}

bool PhysicalDevice::isSwapChainAdequate() {
    return !swapchainsupport.formats.empty() && !swapchainsupport.presentModes.empty();
}

// Chooses the best surface format (color format & quality) for the swap chain
VkSurfaceFormatKHR PhysicalDevice::chooseSwapSurfaceFormat() {
    // Searches for the preferred format, otherwise returns the first available
    // Could be improved by ranking the formats instead, but it should be fine this way
    for (const VkSurfaceFormatKHR& availableFormat: swapchainsupport.formats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }
    return swapchainsupport.formats[0];
}

// Chooses the best present mode (queueing mode) for the swap chain
VkPresentModeKHR PhysicalDevice::chooseSwapPresentMode() {
    if (_forcedpresentmode.has_value())

    // MAILBOX_KHR is preferred, but FIFO_KHR guaranteed to be available and works.
    for (const auto& availablePresentMode : swapchainsupport.presentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

// Ensures a swap mode is chosen, regardless of if the swap mode is support according to the graphics driver.
void PhysicalDevice::forcePresentMode(std::optional<VkPresentModeKHR> presentMode) {
    _forcedpresentmode = presentMode;
    if (presentMode.has_value()) {
        swappresentmode = presentMode.value();
    } else {
        swappresentmode = chooseSwapPresentMode();
    }
}

// This is expected to change if the surface (an abstraction of a window) changes.
VkExtent2D PhysicalDevice::getSwapExtent(VkSurfaceKHR surface, int framebufferwidth, int framebufferheight) {
    VkSurfaceCapabilitiesKHR capabilities = querySwapChainSupportDetails(surface).capabilities;
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    VkExtent2D actualExtent = {
            static_cast<uint32_t>(framebufferwidth),
            static_cast<uint32_t>(framebufferheight)
    };

    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.height);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);


    return actualExtent;
}

VkSampleCountFlagBits PhysicalDevice::getMaxUsableSampleCount() {
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(physicaldevice, &physicalDeviceProperties);

    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}

uint32_t PhysicalDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicaldevice, &memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
        if (typeFilter & (1 << i)
            && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}

VkFormat PhysicalDevice::findSupportedFormat(const vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat formatCandidate: candidates) {
        VkFormatProperties candidateFormatProperties;
        vkGetPhysicalDeviceFormatProperties(physicaldevice, formatCandidate, &candidateFormatProperties);

        if ((tiling == VK_IMAGE_TILING_LINEAR && (candidateFormatProperties.linearTilingFeatures & features) == features)
        || (tiling == VK_IMAGE_TILING_OPTIMAL && (candidateFormatProperties.optimalTilingFeatures & features) == features)) {
            return formatCandidate;
        }
    }

    throw std::runtime_error("Failed to find a supported format!");
}


VkFormat PhysicalDevice::findDepthFormat() {
    try {
        return findSupportedFormat(
                {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                VK_IMAGE_TILING_OPTIMAL,
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    } catch (std::runtime_error& error) {
        throw std::runtime_error("Targeted depth format is not supported!");
    }

}


// Returns a score on how good the GPU if it has the necessary properties/features. Otherwise, returns -1
int PhysicalDevice::rateSuitability() {
    int suitabilityScore = 0;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;

    vkGetPhysicalDeviceProperties(physicaldevice, &properties);
    vkGetPhysicalDeviceFeatures(physicaldevice, &features);

    switch (properties.deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            suitabilityScore += 1000;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            suitabilityScore += 100;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            suitabilityScore += 10;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            suitabilityScore += 1;
        default:
            break;
    }

    bool requiredQueuesSupported = queuefamilies.isComplete();
    bool requiredExtensionsSupported = checkDeviceExtensionSupport();
    bool requiredSwapChainSupported = isSwapChainAdequate();

    if ( !(requiredQueuesSupported && requiredExtensionsSupported && requiredSwapChainSupported) ) {
        return -1;
    }
    return suitabilityScore;
}

void PhysicalDevice::evaluate(VkSurfaceKHR surface) {
    queuefamilies = findQueueFamilies(surface);
    swapchainsupport = querySwapChainSupportDetails(surface);

    score = rateSuitability();

    swappresentmode = chooseSwapPresentMode();
    swapsurfaceformat = chooseSwapSurfaceFormat();
    msaasamples = getMaxUsableSampleCount();
}
