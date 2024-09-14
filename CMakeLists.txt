cmake_minimum_required(VERSION 3.28)
project(ArbitraryFieldControl)

set(CMAKE_CXX_STANDARD 17)

# Packages
find_package(Vulkan REQUIRED)

IF (WIN32)
    set(GLFW_LIB_PATH "$ENV{USERPROFILE}/Desktop/Libraries/glfw-3.4.bin.WIN64/lib-mingw-w64/libglfw3.a")
    set(GLFW_INCLUDE_DIR "$ENV{USERPROFILE}/Desktop/Libraries/glfw-3.4.bin.WIN64/include/")

    add_library(glfw STATIC IMPORTED)
    set_target_properties(glfw PROPERTIES
            IMPORTED_LOCATION ${GLFW_LIB_PATH}
            INTERFACE_INCLUDE_DIRECTORIES ${GLFW_INCLUDE_DIR})
ELSE()
    find_package(glfw3 REQUIRED)
ENDIF()


# Sources
add_executable(ArbitraryFieldControl
        src/main.cpp
        renderer/engine.cpp
        renderer/engine.hpp
        renderer/window.cpp
        renderer/window.hpp
        renderer/pipeline.cpp
        renderer/pipeline.hpp
        renderer/swapchain.cpp
        renderer/swapchain.hpp
        renderer/vulkan_tools.hpp
        renderer/physicaldevice.hpp
        renderer/physicaldevice.cpp
        renderer/vulkan_tools.cpp
        renderer/buffer.cpp
        renderer/buffer.hpp
)
target_include_directories(ArbitraryFieldControl PRIVATE ${CMAKE_SOURCE_DIR})

IF (WIN32)
    set(GLM_INCLUDE_DIR "$ENV{USERPROFILE}/Desktop/Libraries/glm-1.0.1-light")

    target_include_directories(ArbitraryFieldControl PRIVATE ${GLM_INCLUDE_DIR})
ENDIF()


    # Linking
target_link_libraries(ArbitraryFieldControl PRIVATE Vulkan::Vulkan glfw)