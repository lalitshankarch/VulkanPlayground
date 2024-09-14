#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <VkBootstrap.h>
#include <spdlog/spdlog.h>

const uint32_t width = 640, height = 480;

// Throws std::runtime_error on nullptr or falsy values
void check(const auto &ptr, const std::string &msg)
{
#ifdef NDEBUG
    (void)ptr;
    (void)msg;
#else
    if (!ptr)
    {
        spdlog::error(msg);
        throw std::runtime_error(msg);
    }
#endif
}

#ifdef _WIN32
#ifdef NDEBUG
#include <windows.h>
#define main() WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#endif
#endif
int main()
{
    vkb::InstanceBuilder builder{};
#ifdef NDEBUG
    auto inst_ret =
        builder.set_app_name("HelloTriangle").request_validation_layers().build();
#else
    auto inst_ret = builder.set_app_name("HelloTriangle")
                        .request_validation_layers()
                        .use_default_debug_messenger()
                        .build();
#endif
    check(inst_ret, "Vulkan: Failed to create instance");
    vkb::Instance vkb_inst = inst_ret.value();

    check(glfwInit(), "GLFW: Failed to initialize");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(
        width, height, "VulkanPlayground: HelloTriangle", nullptr, nullptr);
    check(window, "GLFW: Failed to create window");

    vk::SurfaceKHR surface{};
    auto glfw_ret =
        glfwCreateWindowSurface(vkb_inst.instance, window, nullptr,
                                reinterpret_cast<VkSurfaceKHR *>(&surface));
    check(glfw_ret == VK_SUCCESS, "GLFW: Failed to create window surface");

    vkb::PhysicalDeviceSelector phys_dev_selector{vkb_inst};
    auto phys_ret = phys_dev_selector.set_surface(surface)
                        .set_minimum_version(1, 3)
                        .select();
    check(phys_ret, "Vulkan: Failed to select physical device");

    vkb::DeviceBuilder dev_builder{phys_ret.value()};
    auto dev_ret = dev_builder.build();
    check(dev_ret, "Vulkan: Failed to create logical device");
    vkb::Device vkb_dev = dev_ret.value();

    auto gr_queue_ret = vkb_dev.get_queue(vkb::QueueType::graphics);
    check(gr_queue_ret, "Vulkan: Failed to get graphics queue");
    vk::Queue graphics_queue = gr_queue_ret.value();

    vkb::SwapchainBuilder swapchain_builder{vkb_dev};
    auto swap_ret = swapchain_builder.build();
    check(swap_ret, "Vulkan: Failed to create swapchain");
    vkb::Swapchain vkb_swapchain = swap_ret.value();

    auto images = vkb_swapchain.get_images().value();
    auto image_views = vkb_swapchain.get_image_views().value();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    vkb_swapchain.destroy_image_views(image_views);
    vkb::destroy_swapchain(vkb_swapchain);
    vkb::destroy_device(vkb_dev);
    vkb::destroy_surface(vkb_inst, surface);
    vkb::destroy_instance(vkb_inst);
    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}
