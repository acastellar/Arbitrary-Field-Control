#include "swapchain.hpp"

using std::string, std::vector;

void SwapChain::createImage(uint32_t width, uint32_t height, VkSampleCountFlagBits numSamples,
                            VkFormat imageFormat, VkImageTiling tiling, VkImageUsageFlags usage,
                            VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent.width = width;
    imageCreateInfo.extent.height = height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.format = imageFormat;
    imageCreateInfo.tiling = tiling;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = usage;
    imageCreateInfo.samples = numSamples;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult create_image_result = vkCreateImage(_device, &imageCreateInfo, nullptr, &image);
    if (create_image_result != VK_SUCCESS) {
        throw vulkan_error("Failed to create image!", create_image_result);
    }

    VkMemoryRequirements imageMemoryRequirements;
    vkGetImageMemoryRequirements(_device, image, &imageMemoryRequirements);

    VkMemoryAllocateInfo imageMemoryAllocationInfo = {};
    imageMemoryAllocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    imageMemoryAllocationInfo.allocationSize = imageMemoryRequirements.size;


    imageMemoryAllocationInfo.memoryTypeIndex = _physicaldevice->findMemoryType(imageMemoryRequirements.memoryTypeBits, properties);

    VkResult image_memory_allocation_result = vkAllocateMemory(_device, &imageMemoryAllocationInfo, nullptr, &imageMemory);
    if (image_memory_allocation_result != VK_SUCCESS) {
        throw vulkan_error("Failed to allocate image memory!", image_memory_allocation_result);
    }

    vkBindImageMemory(_device, image, imageMemory, 0);
}

VkImageView SwapChain::createImageView(VkImage image, VkFormat imageFormat, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo imageViewCreateInfo = {};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.image = image;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = imageFormat;
    imageViewCreateInfo.subresourceRange.aspectMask = aspectFlags;
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    VkResult image_view_creation_result = vkCreateImageView(_device, &imageViewCreateInfo, nullptr, &imageView);
    if (image_view_creation_result != VK_SUCCESS) {
        throw vulkan_error("Failed to create image view!", image_view_creation_result);
    }

    return imageView;
}

void SwapChain::create(VkRenderPass renderpass, int framebufferwidth, int framebufferheight) {
    extent = _physicaldevice->getSwapExtent(_surface, framebufferwidth, framebufferheight);
    format = _physicaldevice->swapsurfaceformat.format;
    depthformat = _physicaldevice->findDepthFormat();

    createSwapChain();
    createImageViews();
    createColorResources();
    createDepthResources();
    createFramebuffers(renderpass);
}

void SwapChain::createSwapChain() {
    const SwapChainSupportDetails& swapChainSupportDetails = _physicaldevice->swapchainsupport;

    uint32_t imageCount = swapChainSupportDetails.capabilities.minImageCount + 1;
    if (swapChainSupportDetails.capabilities.maxImageCount > 0 && imageCount > swapChainSupportDetails.capabilities.maxImageCount) {
        imageCount = swapChainSupportDetails.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = _surface;
    swapChainCreateInfo.imageFormat = format;
    swapChainCreateInfo.imageColorSpace = _physicaldevice->swapsurfaceformat.colorSpace;
    swapChainCreateInfo.imageExtent = extent;
    swapChainCreateInfo.minImageCount = imageCount;
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    const QueueFamilyIndices& queueFamilyIndices = _physicaldevice->queuefamilies;
    uint32_t queueFamilyIndexValues[] = {queueFamilyIndices.graphicsComputeFamily.value(), queueFamilyIndices.presentFamily.value()};

    if (queueFamilyIndices.graphicsComputeFamily.value() != queueFamilyIndices.presentFamily.value()) {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapChainCreateInfo.queueFamilyIndexCount = 2;
        swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndexValues;
    } else {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapChainCreateInfo.queueFamilyIndexCount = 0;
        swapChainCreateInfo.pQueueFamilyIndices = nullptr;
    }

    // Specifies no transformation by setting as current transformation
    swapChainCreateInfo.preTransform = swapChainSupportDetails.capabilities.currentTransform;

    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainCreateInfo.presentMode = _physicaldevice->swappresentmode;
    swapChainCreateInfo.clipped = VK_TRUE;

    swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult swapchain_creation_result = vkCreateSwapchainKHR(_device, &swapChainCreateInfo, nullptr, &swapchain);
    if (swapchain_creation_result != VK_SUCCESS) {
        throw vulkan_error("Failed to create vulkan swap chain!", swapchain_creation_result);
    }

    vkGetSwapchainImagesKHR(_device, swapchain, &imageCount, nullptr);

    images.resize(imageCount);
    vkGetSwapchainImagesKHR(_device, swapchain, &imageCount, images.data());
}

void SwapChain::createImageViews() {
    imageviews.resize(images.size());

    for (size_t i = 0; i < imageviews.size(); i++) {
        imageviews[i] = createImageView(images[i], format, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void SwapChain::createColorResources() {
    VkFormat colorFormat = format;

    createImage(extent.width, extent.height, _physicaldevice->msaasamples, colorFormat,
                VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorimage, colorimagememory);
    colorimageview = createImageView(colorimage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT);
}

void SwapChain::createDepthResources() {
    createImage(extent.width, extent.height, _physicaldevice->msaasamples, depthformat,
                VK_IMAGE_TILING_OPTIMAL,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,depthimage, depthimagememory);
    depthimageview = createImageView(depthimage, depthformat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void SwapChain::createFramebuffers(VkRenderPass renderpass) {
    framebuffers.resize(imageviews.size());

    for (size_t i = 0; i < imageviews.size(); i++) {
        std::array<VkImageView, 3> framebufferAttachments = {
                colorimageview,
                depthimageview,
                imageviews[i]
        };

        VkFramebufferCreateInfo framebufferCreateInfo = {};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = renderpass;
        framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(framebufferAttachments.size());
        framebufferCreateInfo.pAttachments = framebufferAttachments.data();
        framebufferCreateInfo.width = extent.width;
        framebufferCreateInfo.height = extent.height;
        framebufferCreateInfo.layers = 1;

        VkResult framebuffer_creation_result = vkCreateFramebuffer(_device, &framebufferCreateInfo, nullptr, &framebuffers[i]);
        if (framebuffer_creation_result != VK_SUCCESS) {
            throw vulkan_error("Failed to create a framebuffer!", framebuffer_creation_result);
        }
    }
}

SwapChain::SwapChain()
: _device(nullptr), _surface(nullptr), _physicaldevice(nullptr), swapchain(nullptr) {};

SwapChain::SwapChain(VkDevice device, VkSurfaceKHR surface, PhysicalDevice* physicalDevice)
: _device(device), _surface(surface), _physicaldevice(physicalDevice), swapchain(nullptr) {};

SwapChain::~SwapChain() {
    for (VkFramebuffer& framebuffer: framebuffers) {
        vkDestroyFramebuffer(_device, framebuffer, nullptr);
    }
    for (VkImageView& imageView: imageviews) {
        vkDestroyImageView(_device, imageView, nullptr);
    }

    vkDestroyImageView(_device, colorimageview, nullptr);
    vkDestroyImage(_device, colorimage, nullptr);
    vkFreeMemory(_device, colorimagememory, nullptr);

    vkDestroyImageView(_device, depthimageview, nullptr);
    vkDestroyImage(_device, depthimage, nullptr);
    vkFreeMemory(_device, depthimagememory, nullptr);

    vkDestroySwapchainKHR(_device, swapchain, nullptr);
}