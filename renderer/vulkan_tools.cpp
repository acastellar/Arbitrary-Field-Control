#include "vulkan_tools.hpp"

std::runtime_error vulkan_error(const std::string& message, VkResult error) {
    return std::runtime_error(message + " VkResult " + std::to_string(error));
}
