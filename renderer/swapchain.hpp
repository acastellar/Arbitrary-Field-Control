#pragma once

#include "vulkan_tools.hpp"
#include "physicaldevice.hpp"

class SwapChain {
private:
    VkDevice _device;
    VkSurfaceKHR _surface;
    PhysicalDevice* _physicaldevice;

    void createImage(uint32_t width, uint32_t height, VkSampleCountFlagBits numSamples, VkFormat imageFormat,
                     VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                     VkImage& image, VkDeviceMemory& imageMemory);
    VkImageView createImageView(VkImage image, VkFormat imageFormat, VkImageAspectFlags aspectFlags);

    void createSwapChain();
    void createImageViews();
    void createColorResources();
    void createDepthResources();
    void createFramebuffers(VkRenderPass renderpass);

public:
    VkSwapchainKHR swapchain;
    VkFormat format;
    VkExtent2D extent;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageviews;
    std::vector<VkFramebuffer> framebuffers;

    VkFormat depthformat;
    VkImage depthimage;
    VkImageView depthimageview;
    VkDeviceMemory depthimagememory;

    VkImage colorimage;
    VkDeviceMemory colorimagememory;
    VkImageView colorimageview;

    void create(VkRenderPass renderpass, int framebufferWidth, int framebufferHeight);

    SwapChain();
    SwapChain(VkDevice device, VkSurfaceKHR surface, PhysicalDevice* physicalDevice);

    ~SwapChain();
};