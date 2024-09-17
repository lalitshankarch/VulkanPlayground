#pragma once

#include <string>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace lve
{
    class LveWindow
    {
    public:
        LveWindow(int width, int height, std::string name);
        ~LveWindow();
        bool shouldClose();
        void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);
        VkExtent2D getExtent() { return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}; }

        LveWindow(const LveWindow &) = delete;
        LveWindow &operator=(const LveWindow &) = delete;

    private:
        const int width{}, height{};
        GLFWwindow *window{};
        std::string windowName;
        void initWindow();
    };
}