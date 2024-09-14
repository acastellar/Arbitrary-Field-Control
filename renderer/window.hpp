#pragma once

#include "vulkan_tools.hpp"

class Window {
private:
    std::string _name;
    GLFWwindow* _glfwWindow;
    GLFWmonitor* _glfwMonitor;

    void (*_resizecallback)(void* selfpointer, int width, int height) = nullptr;
    void* _resizecallbackselfpointer = nullptr;
public:
    Window(std::string windowName);
    ~Window();

    void init();
    void update();
    void cleanup();

    void setResizeCallback(void* selfpointer, void (*resizeCallback)(void* selfpointer, int width, int height));
    void resizedCallback(int width, int height);

    bool windowShouldClose();
    double getWindowTime();
    void getSizeScreenCoordinates(int& height, int& width);
    void getSizePixels(int& height, int& width);

    VkInstanceCreateInfo getVulkanInstanceCreateInfo();
    void createVulkanSurface(VkInstance instance, VkSurfaceKHR& surface);
};