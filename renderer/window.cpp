#include "window.hpp"

using std::string;

static int glfwWindowCount = 0;

static void windowResizedCallbackGLFW(GLFWwindow* glfwWindow, int width, int height) {
    Window* window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
    if (window) {
        window->resizedCallback(width, height);
    }
}

Window::Window(string windowName) {
    _name = windowName;
    _glfwWindow = nullptr;
    _glfwMonitor = nullptr;
}

void Window::init() {
    glfwInit();

    _glfwMonitor = glfwGetPrimaryMonitor();
    if (!_glfwMonitor) {
        throw std::runtime_error("Could not get the primary monitor!");
    }

    const GLFWvidmode* monitorMode = glfwGetVideoMode(_glfwMonitor);
    if (!monitorMode) {
        throw std::runtime_error("Could not get the monitor's video mode!");
    }

    glfwWindowHint(GLFW_RED_BITS, monitorMode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, monitorMode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, monitorMode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, monitorMode->refreshRate);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    int width = monitorMode->width;
    int height = monitorMode->height;

    _glfwWindow = glfwCreateWindow(width, height, _name.c_str(), nullptr, nullptr);
    if (_glfwWindow) {
        glfwWindowCount++;
    } else {
        throw std::runtime_error("Could not create a new window!");
    }

    glfwSetWindowUserPointer(_glfwWindow, this);
    glfwSetFramebufferSizeCallback(_glfwWindow, windowResizedCallbackGLFW);
}

void Window::setResizeCallback(void* selfpointer, void (*resizeCallback)(void* selfpointer, int width, int height)) {
    _resizecallback = resizeCallback;
    _resizecallbackselfpointer = selfpointer;
}

void Window::resizedCallback(int width, int height) {
    _resizecallback(_resizecallbackselfpointer, width, height);
}

void Window::update() {
    glfwPollEvents();
}

bool Window::windowShouldClose() {
    return glfwWindowShouldClose(_glfwWindow);
}

double Window::getWindowTime() {
    return glfwGetTime();
}

VkInstanceCreateInfo Window::getVulkanInstanceCreateInfo() {
    uint32_t extensionCount;
    const char** extensionNames = glfwGetRequiredInstanceExtensions(&extensionCount);

    VkInstanceCreateInfo createInfo = {};
    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = extensionNames;
    return createInfo;
}

void Window::createVulkanSurface(VkInstance instance, VkSurfaceKHR& surface) {
    VkResult window_surface_creation_result = glfwCreateWindowSurface(instance, _glfwWindow, nullptr, &surface);
    if (window_surface_creation_result != VK_SUCCESS) {
        throw vulkan_error("Failed to create vulkan surface with GLFW!", window_surface_creation_result);
    }
}

void Window::getSizeScreenCoordinates(int& width, int& height) {
    glfwGetWindowSize(_glfwWindow, &width, &height);
}

// Gets the window size in pixels. This is also the framebuffer size.
void Window::getSizePixels(int& width, int& height) {
    glfwGetFramebufferSize(_glfwWindow, &width, &height);
}

void Window::cleanup() {
    if (_glfwWindow) {
        glfwDestroyWindow(_glfwWindow);
        _glfwWindow = nullptr;
        glfwWindowCount--;
    }

    if (glfwWindowCount == 0) {
        glfwTerminate();
    }
}

Window::~Window() {
    cleanup();
}

